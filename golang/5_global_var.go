package main

import (
	"fmt"
)

func initRedis() string {
	fmt.Println("Initializing Redis connection...")
	return "Redis instance"
}

// 全局变量的初始化在main之前
var instance = initRedis()

func main() {
	fmt.Println("Main function started.")
	fmt.Println("Instance:", instance)
}
