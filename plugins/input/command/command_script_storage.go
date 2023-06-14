package command

import (
	"bufio"
	"context"
	"fmt"
	"os"
	"os/exec"
	"strings"
	"sync"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type ScriptStorage struct {
	StorageDir  string
	FilePathMap sync.Map
	Err         error
}

var storageOnce sync.Once
var storageInstance *ScriptStorage

func GetStorage(dataDir string) *ScriptStorage {
	dataDir = strings.Trim(dataDir, " ")
	storageOnce.Do(func() {
		storageInstance = &ScriptStorage{
			StorageDir: dataDir,
		}
		storageInstance.Init()
	})
	return storageInstance
}

func PathExists(path string) (bool, error) {
	_, err := os.Stat(path)
	if err == nil {
		return true, nil
	}
	if os.IsNotExist(err) {
		return false, nil
	}
	return false, err
}

func (storage *ScriptStorage) Init() {
	//创建目录
	//查询用户是否存在
	isExist, err := PathExists(storage.StorageDir)
	fmt.Print("\n", "###", err, storage.StorageDir, "\n")
	if isExist {
		return
	}
	cmd := exec.Command("mkdir", "-p", storage.StorageDir)
	output, err := cmd.Output()
	if err != nil {
		fmt.Print("\n\n", "----mkdir path ---", string(output), "\n\n")
		storage.Err = fmt.Errorf("Execute mkdir -p %s failed with error:%s", storage.StorageDir, err.Error())
		return
	}
}

///temp/  dir1  dir   => /temp/dir1/dir2
func (storage *ScriptStorage) mkdirSubPath(dirs ...string) (string, bool) {
	stdPath := strings.TrimRight(storage.StorageDir, "/")
	dirPath := make([]string, 0)
	dirPath = append(dirPath, stdPath)
	dirPath = append(dirPath, dirs...)
	path := strings.Join(dirPath, "/")
	isExist, _ := PathExists(path)
	if isExist {
		return path, true
	}

	cmd := exec.Command("mkdir", "-p", path)

	output, err := cmd.Output()
	if err != nil {
		fmt.Print("\n\n", "----mkdir path ---", string(output), "\n\n")
		storage.Err = fmt.Errorf("error occurred on mkdir subpath %s dir:%s", err, path)
		return "", false
	}
	return path, true
}

func (storage *ScriptStorage) SaveContent(content string) (string, error) {
	md5Str := getContentMd5(content)
	if filePathInterface, ok := storage.FilePathMap.Load(md5Str); ok {
		filepath, ok := filePathInterface.(string)
		if !ok {
			return "", fmt.Errorf("get filepath from filepathmap error %+v", filepath)
		}
		return filepath, nil
	}

	//保存脚本到机器上
	dir1 := md5Str[0:4]
	dir2 := md5Str[4:8]
	path, re := storage.mkdirSubPath(dir1, dir2)
	if !re {
		return "", fmt.Errorf("error occurred on mkdirSubPath err:%s", storage.Err)
	}
	filePath := fmt.Sprintf("%s/%s.sh", path, md5Str)
	file, err := os.OpenFile(filePath, os.O_WRONLY|os.O_CREATE, 0755)
	if err != nil {
		fmt.Println("文件打开失败", err)
	}
	defer file.Close()
	//写入文件时，使用带缓存的 *Writer
	write := bufio.NewWriter(file)
	write.WriteString(content)
	//Flush将缓存的文件真正写入到文件中
	write.Flush()
	logger.Debug(context.Background(), "[input command plugin] ScriptStorage write file Success path", filePath, "content", content)
	fmt.Println("[input command plugin] ScriptStorage write file Success ", filePath)

	//写入FilePathMap记录
	storage.FilePathMap.Store(md5Str, filePath)
	return filePath, nil
}
