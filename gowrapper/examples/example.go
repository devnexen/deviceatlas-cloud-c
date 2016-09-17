package main

import "dacloud"
import "fmt"
import "os"

func main() {
	if len(os.Args) < 3 {
		print("Needs the config file path and an user-agent\n");
		os.Exit(0);
	}
	confpath := os.Args[1]
	ua := os.Args[2]
	ret := dacloud.Init(confpath);
	if ret.R == 0 {
		m := make(map[string]string);
		m["user-agent"] = ua;
		props := dacloud.Detect(ret, m);
		for k, v := range props {
			fmt.Printf("%s => ", k);
			fmt.Println(v);
		}
	}
	dacloud.Finalize(ret);
}
