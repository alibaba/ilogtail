package config

import (
	"config-server/common"
	"config-server/utils"
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

var config = new(GormConfig)

func Connect2Db() (*GormConfig, gorm.Dialector, error) {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/?charset=utf8&parseTime=True&loc=Local",
		config.UserName,
		config.Password,
		config.Host,
		config.Port)
	return getConnection(dsn)
}

func Connect2SpecifiedDb() (*GormConfig, gorm.Dialector, error) {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=utf8&parseTime=True&loc=Local",
		config.UserName,
		config.Password,
		config.Host,
		config.Port,
		config.DbName)
	return getConnection(dsn)
}

func readDatabaseConfig() error {
	var err error
	envName, err := utils.GetEnvName()
	if err != nil {
		return err
	}
	databaseConfigPath, err := filepath.Abs(fmt.Sprintf("cmd/config/%s/databaseConfig.json", envName))
	if err != nil {
		return err
	}
	err = utils.ReadJson(databaseConfigPath, config)
	if err != nil {
		return err
	}
	return err
}

func getConnection(dsn string) (*GormConfig, gorm.Dialector, error) {
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

func init() {
	err := readDatabaseConfig()
	if err != nil {
		panic(err)
	}
}
