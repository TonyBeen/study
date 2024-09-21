package main

import (
	"fmt"
	"unsafe"
)

func swap(x, y string) (string, string) {
	return y, x
}

func print_string(obj *string, msg string) {
	string_ptr := (*[2]uintptr)(unsafe.Pointer(obj))
	first_obj_addr := string_ptr[0] // 存储字符串地址的对象

	first_byte := (*[5]uint8)(unsafe.Pointer(first_obj_addr)) // 将uintptr转成可解引用的地址
	fmt.Printf("%s\n", msg)
	fmt.Printf("\t%p\n", first_byte)
	fmt.Printf("\t%c\n", *first_byte)

	// first_byte[0] = 'E'
}

func test_swap() {
	var a, b string = "Hello", "World"

	fmt.Println(unsafe.Sizeof(string("")))

	print_string(&a, "-------a-------")
	print_string(&b, "-------b-------")

	a, b = swap(a, b)

	print_string(&a, "-------a-------")
	print_string(&b, "-------b-------")

	// 经过上述测试, 字符串结果为两个变量, 第一个地址, 第二个长度
	// sizeof(string)大小为16
	// swap的本质类似于std::move, 将a和b的内部变量进行了交换, 并没有将字符串进行拷贝
}

func main() {
	test_swap()
	for {

	}
}
