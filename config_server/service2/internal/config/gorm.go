package config

import (
	"config-server2/internal/utils"
	"fmt"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
	"path/filepath"
	"runtime"
)

type GormConfig struct {
	Type        string `json:"type"`
	UserName    string `json:"userName"`
	Password    string `json:"password"`
	Host        string `json:"host"`
	Port        int32  `json:"port"`
	DbName      string `json:"dbName"`
	AutoMigrate bool   `json:"autoMigrate"`
}

var gormDialectMap = map[string]func(string) gorm.Dialector{
	"mysql": mysql.Open,
}

func GetConnection() (*GormConfig, gorm.Dialector) {
	var config = new(GormConfig)
	_, currentFilePath, _, _ := runtime.Caller(0)
	err := utils.ReadJson(filepath.Join(filepath.Dir(currentFilePath), "./databaseConfig.json"), config)
	if err != nil {
		return nil, nil
	}
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=utf8&parseTime=True&loc=Local",
		config.UserName,
		config.Password,
		config.Host,
		config.Port,
		config.DbName)

	if dialect, ok := gormDialectMap[config.Type]; ok {
		return config, dialect(dsn)
	}
	panic("no this database type")
}
