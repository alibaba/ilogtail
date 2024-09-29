package repository

import (
	"config-server/internal/common"
	"gorm.io/gorm/clause"
)

func generateClauseColumn(names ...string) []clause.Column {
	if names == nil {
		return nil
	}
	arr := make([]clause.Column, 0)
	for _, name := range names {
		arr = append(arr, clause.Column{
			Name: name,
		})
	}
	return arr
}

func createOrUpdateEntities[T any](conflictColumnNames []string, assignmentColumns []string, entities ...T) error {
	if conflictColumnNames == nil {
		return common.ServerErrorWithMsg("conflictColumnNames could not be null")
	}
	if entities == nil || len(entities) == 0 {
		return nil
	}

	//如果原样插入，即数据没有任何变化，RowsAffected返回的值是0，所以这里只能用error来判断是否发生插入或更新异常
	columns := generateClauseColumn(conflictColumnNames...)
	if assignmentColumns == nil {
		err := s.Db.Clauses(clause.OnConflict{
			Columns:   columns,
			UpdateAll: true,
		}).Create(&entities).Error
		return common.SystemError(err)
	}

	err := s.Db.Clauses(clause.OnConflict{
		Columns:   columns,                                     // 指定冲突的列
		DoUpdates: clause.AssignmentColumns(assignmentColumns), // 如果冲突发生，更新的列
	}).Create(&entities).Error
	return common.SystemError(err)
}
