package config

import (
	"config-server/internal/common"
	"config-server/internal/utils"
	"fmt"
	"gorm.io/driver/mysql"
	"gorm.io/driver/postgres"
	"gorm.io/driver/sqlite"
	"gorm.io/driver/sqlserver"
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
	"mysql":     mysql.Open,
	"sqlite":    sqlite.Open,
	"sqlserver": sqlserver.Open,
	"postgres":  postgres.Open,
}

func GetConnection() (*GormConfig, gorm.Dialector, error) {
	var config = new(GormConfig)
	var err error
	envName, err := utils.GetEnvName()
	if err != nil {
		return nil, nil, err
	}
	databaseConfigPath, err := filepath.Abs(fmt.Sprintf("cmd/config/%s/databaseConfig.json", envName))
	if err != nil {
		return nil, nil, err
	}
	err = utils.ReadJson(databaseConfigPath, config)
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
	adapterDatabaseStr := ""
	for adapterDatabase := range gormDialectMap {
		adapterDatabaseStr += adapterDatabase
		adapterDatabaseStr += ","
	}

	panic(fmt.Sprintf("no this database type in (%s)", adapterDatabaseStr[:len(adapterDatabaseStr)-1]))
}
