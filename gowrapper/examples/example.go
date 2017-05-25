package main

import (
	"dacloud"
	"fmt"
	"os"
	"sync"
	"sync/atomic"
)

var cnt uint32

func main() {
	if len(os.Args) < 3 {
		print("Needs the config file path and an user-agent\n")
		os.Exit(0)
	}
	confpath := os.Args[1]
	ua := os.Args[2]
	wg := new(sync.WaitGroup)
	cnt = 0
	ret := dacloud.Init(confpath)
	defer ret.Finalize()
	if ret.R == 0 {
		m := make(map[string]string)
		m["user-agent"] = ua
		if props, err := ret.Detect(m); err == nil {
			var i int
			fmt.Println("\none goroutine mode")
			for k, v := range props {
				fmt.Printf("%s => ", k)
				fmt.Println(v)
			}
			fmt.Println("\ngoroutine mode")
			wg.Add(10)
			for i = 0; i < 10; i++ {
				go func(ret *dacloud.DaGo, m map[string]string) {
					g := atomic.AddUint32(&cnt, 1)
					fmt.Printf("goroutine %d: before loop\n", g)
					_, _ = ret.Detect(m)
					fmt.Printf("goroutine %d: after loop\n", g)
					wg.Done()
				}(ret, m)
			}
			wg.Wait()
			fmt.Printf("%d concurrent lookups\n", cnt)
		} else {
			fmt.Printf("%s\n", err)
		}
	}
}
