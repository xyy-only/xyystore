package ebpf

import (
	"bytes"
	"fmt"
	"net"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
)

// 定义eBPF程序和映射
const netStatsBPF = `
// SPDX-License-Identifier: GPL-2.0
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

#define TC_ACT_OK       0
#define TC_ACT_SHOT     2
#define TC_ACT_UNSPEC   -1

struct net_stats {
    __u64 rcv_bytes;
    __u64 rcv_packets;
    __u64 snd_bytes;
    __u64 snd_packets;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 64);
    __type(key, __u32);
    __type(value, struct net_stats);
} net_stats_map SEC(".maps");

static __always_inline void update_stats(__u32 ifindex, __u32 len, bool is_rx)
{
    struct net_stats *stats;
    struct net_stats new_stats = {};

    stats = bpf_map_lookup_elem(&net_stats_map, &ifindex);
    if (!stats) {
        if (is_rx) {
            new_stats.rcv_bytes = len;
            new_stats.rcv_packets = 1;
        } else {
            new_stats.snd_bytes = len;
            new_stats.snd_packets = 1;
        }
        bpf_map_update_elem(&net_stats_map, &ifindex, &new_stats, BPF_ANY);
    } else {
        if (is_rx) {
            __sync_fetch_and_add(&stats->rcv_bytes, len);
            __sync_fetch_and_add(&stats->rcv_packets, 1);
        } else {
            __sync_fetch_and_add(&stats->snd_bytes, len);
            __sync_fetch_and_add(&stats->snd_packets, 1);
        }
    }
}

SEC("tc/ingress")
int tc_ingress(struct __sk_buff *skb)
{
    __u32 ifindex = skb->ifindex;
    __u32 len = skb->len;

    if (ifindex == 0 || len == 0)
        return TC_ACT_OK;

    update_stats(ifindex, len, true);

    return TC_ACT_OK;
}

SEC("tc/egress")
int tc_egress(struct __sk_buff *skb)
{
    __u32 ifindex = skb->ifindex;
    __u32 len = skb->len;

    if (ifindex == 0 || len == 0)
        return TC_ACT_OK;

    update_stats(ifindex, len, false);

    return TC_ACT_OK;
}

char LICENSE[] SEC("license") = "GPL";
`

type NetStats struct {
	RcvBytes   uint64
	RcvPackets uint64
	SndBytes   uint64
	SndPackets uint64
}

type NetStatsCollector struct {
	objs     ebpf.Collection
	links    []link.Link
	statsMap *ebpf.Map
}

func NewNetStatsCollector() (*NetStatsCollector, error) {
	// 加载eBPF程序
	spec, err := ebpf.LoadCollectionSpecFromReader(bytes.NewReader([]byte(netStatsBPF)))
	if err != nil {
		return nil, fmt.Errorf("加载eBPF程序失败: %w", err)
	}

	// 创建eBPF对象
	objs := ebpf.Collection{}
	if err := spec.LoadAndAssign(&objs, nil); err != nil {
		return nil, fmt.Errorf("加载eBPF对象失败: %w", err)
	}

	// 获取stats map
	statsMap, ok := objs.Map("net_stats_map")
	if !ok {
		objs.Close()
		return nil, fmt.Errorf("找不到net_stats_map")
	}

	collector := &NetStatsCollector{
		objs:     objs,
		statsMap: statsMap,
		links:    make([]link.Link, 0),
	}

	// 挂载到所有网络接口
	if err := collector.attachToAllInterfaces(); err != nil {
		collector.Close()
		return nil, err
	}

	return collector, nil
}

func (c *NetStatsCollector) attachToAllInterfaces() error {
	// 获取所有网络接口
	interfaces, err := net.Interfaces()
	if err != nil {
		return fmt.Errorf("获取网络接口失败: %w", err)
	}

	// 挂载到每个接口
	for _, iface := range interfaces {
		// 跳过回环接口
		if iface.Flags&net.FlagLoopback != 0 {
			continue
		}

		// 挂载ingress
		ingressLink, err := link.AttachTC(link.TCAttachParams{
			InterfaceIndex: iface.Index,
			Program:        c.objs.Program("tc_ingress"),
			AttachPoint:    link.TCAttachPointIngress,
		})
		if err != nil {
			continue // 跳过无法挂载的接口
		}
		c.links = append(c.links, ingressLink)

		// 挂载egress
		egressLink, err := link.AttachTC(link.TCAttachParams{
			InterfaceIndex: iface.Index,
			Program:        c.objs.Program("tc_egress"),
			AttachPoint:    link.TCAttachPointEgress,
		})
		if err != nil {
			ingressLink.Close()
			continue // 跳过无法挂载的接口
		}
		c.links = append(c.links, egressLink)
	}

	return nil
}

func (c *NetStatsCollector) GetStats() (map[string]NetStats, error) {
	stats := make(map[string]NetStats)

	// 遍历map中的所有条目
	iter := c.statsMap.Iterate()
	var key uint32
	var value NetStats

	for iter.Next(&key, &value) {
		// 通过ifindex获取接口名
		iface, err := net.InterfaceByIndex(int(key))
		if err != nil {
			continue
		}

		stats[iface.Name] = value
	}

	return stats, iter.Err()
}

func (c *NetStatsCollector) ResetStats() error {
	// 遍历map中的所有条目并删除
	iter := c.statsMap.Iterate()
	var key uint32

	for iter.Next(&key, nil) {
		if err := c.statsMap.Delete(&key); err != nil {
			return err
		}
	}

	return iter.Err()
}

func (c *NetStatsCollector) Close() error {
	// 关闭所有链接
	for _, link := range c.links {
		link.Close()
	}

	// 关闭eBPF对象
	return c.objs.Close()
}
