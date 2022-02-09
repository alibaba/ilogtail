package main

func main() {

	bytes := []byte("jfksdjfksfdj")
	a := string(bytes)
	b := string(bytes)
	c := a[0:4]
	bytes1 := []byte(a)
	bytes11 := []byte(a)
	bytes2 := []byte(c)
	bytes22 := []byte(c)
	print(bytes1, bytes11, bytes2, bytes22)
	print(b)

}
