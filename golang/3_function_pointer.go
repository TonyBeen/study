package main

import (
	"fmt"
	"math"
)

// 函数可以值传递和引用传递
func transmit_param(a *uint32, b string) {
	// 此时a是引用, b是值, 修改a会影响外部变量, 修改b则无影响
	*a = 100
	b = "Other string"
}

func call_transmit_param() {
	a := uint32(0)
	b := "something"

	transmit_param(&a, b)
	fmt.Printf("a = %d, b = %s\n", a, b)
}

// 函数可以有多个返回值并且可以命名
func return_multi() (x string, y uint32) {
	str := "HELLO, WORLD!"
	return str, 56
}

func call_return_multi() {
	var x, y = return_multi()
	fmt.Println(x, y)
}

// 函数可以返回闭包
func return_closure() func() uint32 {
	number := uint32(0) // 闭包会影响变量的生命周期

	return func() uint32 {
		number++ // 必须自成一行
		return number
	}
}

func call_return_closure() {
	func_ptr := return_closure()

	fmt.Println(func_ptr()) // number == 1
	fmt.Println(func_ptr()) // number == 2

	other_func_ptr := return_closure() // number会被重置为0

	fmt.Println(func_ptr())       // number == 3
	fmt.Println(other_func_ptr()) // number == 1
	fmt.Println(other_func_ptr()) // number == 2
	fmt.Println(func_ptr())       // number == 4

	// 闭包将number的生命周期进行延长, func_ptr的作用域结束后会释放
}

// 变量可以接收闭包, 类似于lambda
func call_param_closure() {
	getSquareRoot := func(x float64) float64 {
		return math.Sqrt(x)
	}

	fmt.Println(getSquareRoot(9))
}

func main() {
	call_transmit_param()
	call_return_multi()
	call_return_closure()
	call_param_closure()
}
