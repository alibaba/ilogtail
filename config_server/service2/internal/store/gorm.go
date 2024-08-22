package store

import (
	"config-server2/internal/config"
	"config-server2/internal/entity"
	"fmt"
	"gorm.io/gorm"
	"log"
)

var S = new(GormStore)

type GormStore struct {
	config *config.GormConfig
	DB     *gorm.DB
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
	entity.AgentAndAgentGroupTable,

	entity.AgentGroupAndInstanceConfigTable,
	entity.AgentGroupAndPipelineConfigTable,

	entity.Agent{}.TableName(),

	entity.InstanceConfig{}.TableName(),
	entity.PipelineConfig{}.TableName(),
	entity.AgentPipelineConfig{}.TableName(),
	entity.AgentInstanceConfig{}.TableName(),
}

func (s *GormStore) Connect() error {
	var err error
	var dialect gorm.Dialector
	s.config, dialect, err = config.GetConnection()
	if err != nil {
		return err
	}
	s.DB, err = gorm.Open(dialect)
	if err != nil {
		return err
	}
	log.Printf(" %s (%s:%d) connect success...", s.config.Type, s.config.Host, s.config.Port)
	return nil
}

func (s *GormStore) Close() error {
	db, err := s.DB.DB()
	if err != nil {
		return err
	}
	return db.Close()
}

func (s *GormStore) CreateTables() error {
	//AgentPipeConfig和AgentInstanceConfig要额外autoMigrate是因为他们有多余的属性
	err := s.DB.AutoMigrate(tableList...)
	return err
}

func (s *GormStore) DeleteTables() error {
	var err error
	s.DB.Exec("SET FOREIGN_KEY_CHECKS = 0;")
	for _, tableName := range tableNameList {
		if s.DB.Migrator().HasTable(tableName) {

			err = s.DB.Migrator().DropTable(tableName)
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func (s *GormStore) DeleteTable() error {
	var err error
	s.DB.Exec("SET FOREIGN_KEY_CHECKS = 0;")
	for _, tableName := range tableNameList {
		if s.DB.Migrator().HasTable(tableName) {
			var clearDataSql = fmt.Sprintf("TRUNCATE TABLE %s", tableName)
			err = s.DB.Exec(clearDataSql).Error
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func init() {
	var err error
	err = S.Connect()
	if err != nil {
		panic(err)
	}
	if S.config.AutoMigrate {
		err = S.CreateTables()
		if err != nil {
			panic(err)
		}
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
