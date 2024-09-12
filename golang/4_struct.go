package main

import "fmt"

type Books struct {
	title   string
	author  string
	subject string
	book_id int
}

// Go没有类概念
// 为结构体声明方法, 只能这个结构体对象能调用
func (this Books) print() {
	fmt.Printf("Book title : %s\n", this.title)
	fmt.Printf("Book author : %s\n", this.author)
	fmt.Printf("Book subject : %s\n", this.subject)
	fmt.Printf("Book book_id : %d\n\n", this.book_id)
}

// 还可以声明引用方式的方法, 这种方法比较常用, 用于修改结构体成员变量
func (this *Books) SetTitle(title string) {
	this.title = title
}

/*
使用指针接收者的原因有二：

首先，方法能够修改其接收者指向的值。

其次，这样可以避免在每次调用方法时复制该值。若值的类型为大型结构体时，这样会更加高效。
*/

func main() {
	var Book1 Books /* 声明 Book1 为 Books 类型 */
	var Book2 Books /* 声明 Book2 为 Books 类型 */

	/* book 1 描述 */
	Book1.title = "Go 语言"
	Book1.author = "www.runoob.com"
	Book1.subject = "Go 语言教程"
	Book1.book_id = 6495407

	/* book 2 描述 */
	Book2.title = "Python 教程"
	Book2.author = "www.runoob.com"
	Book2.subject = "Python 语言教程"
	Book2.book_id = 6495700

	/* 打印 Book1 信息 */
	Book1.print()

	/* 打印 Book2 信息 */
	Book2.print()

	// 方法与指针重定向
	// 这行代码会被解释为 (&Book1).SetTitle("golang 语言")
	Book1.SetTitle("golang 语言")

	// 同理, 一个指针调用print时会被解释为 (*pBook1).print()
	pBook1 := &Book1
	pBook1.print()
}
