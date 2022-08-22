package common

type (
	Queue struct {
		top    *node
		rear   *node
		length int
	}

	node struct {
		pre   *node
		next  *node
		value interface{}
	}
)

// Create a new queue
func NewQueue() *Queue {
	return &Queue{nil, nil, 0}
}

func (this *Queue) Len() int {
	return this.length
}

func (this *Queue) Empty() bool {
	return this.length == 0
}

func (this *Queue) Front() interface{} {
	if this.top == nil {
		return nil
	}
	return this.top.value
}

func (this *Queue) Push(v interface{}) {
	n := &node{nil, nil, v}
	if this.length == 0 {
		this.top = n
		this.rear = this.top
	} else {
		n.pre = this.rear
		this.rear.next = n
		this.rear = n
	}
	this.length++
}

func (this *Queue) Pop() interface{} {
	if this.length == 0 {
		return nil
	}
	n := this.top
	if this.top.next == nil {
		this.top = nil
	} else {
		this.top = this.top.next
		this.top.pre.next = nil
		this.top.pre = nil
	}
	this.length--
	return n.value
}

func (this *Queue) Clear() {
	for !this.Empty() {
		this.Pop()
	}
}
