package config

import (
	"config-server2/internal/common"
	"config-server2/internal/utils"
	"fmt"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
	"path/filepath"
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

func GetConnection() (*GormConfig, gorm.Dialector, error) {
	var config = new(GormConfig)
	var err error
	dataBaseConfigPath, err := filepath.Abs("cmd/config/dataBaseConfig.json")
	if err != nil {
		return nil, nil, err
	}
	//log.Print(dataBaseConfigPath)
	err = utils.ReadJson(dataBaseConfigPath, config)
	if err != nil {
		return nil, nil, err
	}
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=utf8&parseTime=True&loc=Local",
		config.UserName,
		config.Password,
		config.Host,
		config.Port,
		config.DbName)

	if dialect, ok := gormDialectMap[config.Type]; ok {
		gormDialect := dialect(dsn)
		if gormDialect == nil {
			return nil, nil, common.ServerErrorWithMsg("connect %s:%s dbName:%s failed",
				config.Host, config.Port, config.DbName)
		}
		return config, dialect(dsn), nil
	}
	panic("no this database type")
}
