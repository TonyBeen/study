package main

import (
	"fmt"
	"strconv"
)

// len 返回切片、数组、字符串、映射等的长度或元素数量
func test_len() {
	s := "hello"
	fmt.Println(len(s)) // 输出: 5
	// len不同于strlen, 直接返回长度, 不会计算

	a := []int{1, 2, 3}
	fmt.Println(len(a)) // 输出: 3
}

// cap 返回切片的容量（即切片可以扩展到的最大长度）
func test_cap() {
	s := make([]int, 1)

	fmt.Printf("before cap = %d", cap(s))

	// 两倍方式扩容, append返回值是对象, 数组地址在不进行扩容时都是一致的, 每次append会更改s的内存位置
	for i := 0; i < 20; i++ {
		s = append(s, i)
		fmt.Printf("new_slice = %d, %p,\n", cap(s), &s[0])
	}

	fmt.Println(s) // 输出: 10
}

// make 用于创建切片、映射和通道，并初始化它们的内部数据结构
func test_make() {
	// Go语言也可通过 {}限制作用域, 使得中括号内的变量无法被外部看见
	{
		s := make([]int, 5) // 创建一个长度为 5 的切片
		var _ = s

		var numbers []int // 声明一个空切片
		var _ = numbers
		if numbers == nil {
			fmt.Printf("切片是空的\n")
		}

		// s = make(map[string]int) 类型确定之后无法在修改变量的类型, 此处放开会报错

		m := make(map[uint32]string) // 创建一个空映射, 映射的底层是哈希表
		var _ = m
		m = make(map[uint32]string, 5) // 创建一个初始容量为5的映射
		fmt.Printf("map length = %d, %v\n", len(m), m)

		// NOTE i++只能当做单独语句使用, 无法当做函数参数, 故也就没有前置++和后置++的区别
		for i := 0; i < 26; i++ {
			m[uint32(i)] = strconv.Itoa(i * 10)
		}

		for key, val := range m {
			fmt.Printf("%v --> %v\n", key, val)
		}

		ch := make(chan int) // 创建一个无缓冲的通道

		// 类似于 (void)s, 防止unused报错
		var _ = s
		var _ = m
		var _ = ch
	}
}

// new 分配一个类型的零值，并返回指向这个零值的指针
// new 分配的内存无需手动释放, delete作用是删除元素而不是释放内存
func test_new() {
	p := new(map[uint32]string) // 返回一个指向 map[uint32]string 类型零值的指针
	fmt.Println(*p)             // 输出: map[]

	var other_ptr *map[uint32]string = new(map[uint32]string)
	fmt.Println(*other_ptr) // 输出: map[]
}

// delete 只能从映射中删除一个键值对
func test_delete() {
	m := map[string]int{"a": 1, "b": 2}
	delete(m, "a") // 删除键 "a" 对应的值
	fmt.Println(m) // 输出: map[b:2]
}

func main() {
	test_len()
	test_cap()
	test_make()
	test_new()
	test_delete()
}
