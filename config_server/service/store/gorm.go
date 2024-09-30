package store

import (
	"config-server/config"
	"config-server/entity"
	"fmt"
	"gorm.io/gorm"
	"log"
)

var S = new(GormStore)

type GormStore struct {
	Config *config.GormConfig
	Db     *gorm.DB
}

var tableList = []any{
	&entity.InstanceConfig{},
	&entity.PipelineConfig{},
	&entity.AgentGroup{},
	&entity.Agent{},
	&entity.AgentPipelineConfig{},
	&entity.AgentInstanceConfig{},
}

var tableNameList = []string{
	entity.AgentGroup{}.TableName(),
	entity.AgentAgentGroupTable,

	entity.AgentGroupInstanceConfigTable,
	entity.AgentGroupPipelineConfigTable,

	entity.Agent{}.TableName(),

	entity.InstanceConfig{}.TableName(),
	entity.PipelineConfig{}.TableName(),
	entity.AgentPipelineConfig{}.TableName(),
	entity.AgentInstanceConfig{}.TableName(),
}

func (s *GormStore) Connect2Db() error {
	var err error
	var dialect gorm.Dialector
	s.Config, dialect, err = config.Connect2Db()
	if err != nil {
		return err
	}
	log.Printf("test connect database type=%s host=%s:%d ...",
		s.Config.Type, s.Config.Host, s.Config.Port)
	s.Db, err = gorm.Open(dialect)
	if err != nil {
		return err
	}
	dbName := s.Config.DbName
	log.Printf("create database %s ...", dbName)
	err = s.Db.Exec(fmt.Sprintf("CREATE DATABASE IF NOT EXISTS %s", dbName)).Error
	if err != nil {
		return err
	}
	log.Printf("create database %s success ...", dbName)
	log.Printf("check %s ...", dbName)
	err = s.Db.Exec(fmt.Sprintf("USE %s", s.Config.DbName)).Error
	if err != nil {
		return err
	}
	log.Printf("check %s success", dbName)
	return nil
}

func (s *GormStore) Connect2SpecifiedDb() error {
	var err error
	var dialect gorm.Dialector
	s.Config, dialect, err = config.Connect2SpecifiedDb()
	if err != nil {
		return err
	}
	dbName := s.Config.DbName
	log.Printf("test connect database type=%s host=%s:%d dbName=%s ...",
		s.Config.Type, s.Config.Host, s.Config.Port, dbName)
	s.Db, err = gorm.Open(dialect)
	if err != nil {
		return err
	}
	log.Printf("sucess connect database type=%s host=%s:%d dbName=%s ...",
		s.Config.Type, s.Config.Host, s.Config.Port, dbName)
	return nil
}

func (s *GormStore) Close() error {
	db, err := s.Db.DB()
	if err != nil {
		return err
	}
	return db.Close()
}

func (s *GormStore) CreateTables() error {
	//AgentPipeConfig和AgentInstanceConfig要额外autoMigrate是因为他们有多余的属性
	log.Printf("create tables ...")
	err := s.Db.AutoMigrate(tableList...)
	if err != nil {
		return err
	}
	log.Printf("create tables success ...")
	return nil
}

func (s *GormStore) DeleteTables() error {
	var err error
	s.Db.Exec("SET FOREIGN_KEY_CHECKS = 0;")
	for _, tableName := range tableNameList {
		if s.Db.Migrator().HasTable(tableName) {

			err = s.Db.Migrator().DropTable(tableName)
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func (s *GormStore) DeleteTable() error {
	var err error
	s.Db.Exec("SET FOREIGN_KEY_CHECKS = 0;")
	for _, tableName := range tableNameList {
		if s.Db.Migrator().HasTable(tableName) {
			var clearDataSql = fmt.Sprintf("TRUNCATE TABLE %s", tableName)
			err = s.Db.Exec(clearDataSql).Error
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func init() {
	var err error
	err = S.Connect2Db()
	if err != nil {
		panic(err)
	}
	if S.Config.AutoMigrate {
		err = S.CreateTables()
		if err != nil {
			panic(err)
		}
	}
	err = S.Connect2SpecifiedDb()
	if err != nil {
		panic(err)
	}
}

func Init() {
	var err error
	err = S.CreateTables()
	if err != nil {
		panic(err)
	}
}

func Delete() {
	var err error
	err = S.DeleteTables()
	if err != nil {
		panic(err)
	}
}
