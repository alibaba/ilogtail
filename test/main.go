package main

import (
	"flag"
	"fmt"
	"math/rand"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/test/engine"
	"github.com/cucumber/godog"
)

func main() {
	name := flag.String("test_case", "soak", "test case type")
	flag.Parse()

	for {
		select {
		case <-time.After(5 * time.Minute):
			triggerOneRound(*name)
		}
	}
}

func triggerOneRound(name string) {
	fmt.Println("=====================================")
	fmt.Printf("Trigger one round of %s\n", name)
	// walk all files name endwith .feature in name
	files := make([]string, 0)
	err := filepath.Walk(name, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if strings.HasSuffix(path, ".feature") {
			files = append(files, path)
		}
		return nil
	})
	if err != nil {
		fmt.Println(err)
		return
	}
	fmt.Printf("found %d files\n", len(files))
	rand.Seed(time.Now().UnixNano())
	randomIndex := rand.Intn(len(files))
	fmt.Printf("run %s\n", files[randomIndex])
	fmt.Println("=====================================")

	// run godog
	suite := godog.TestSuite{
		Name:                "Soak",
		ScenarioInitializer: engine.ScenarioInitializer,
		Options: &godog.Options{
			Format: "pretty",
			Paths:  []string{files[randomIndex]},
		},
	}
	if suite.Run() != 0 {
		fmt.Printf("run %s failed\n", files[randomIndex])
		return
	}
}
