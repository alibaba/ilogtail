package repository

import (
	"config-server2/internal/common"
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
	if assignmentColumns == nil {
		err := s.DB.Clauses(clause.OnConflict{
			Columns:   generateClauseColumn(conflictColumnNames...),
			UpdateAll: true,
		}).Create(&entities).Error
		return common.SystemError(err)
	}

	err := s.DB.Clauses(clause.OnConflict{
		Columns:   generateClauseColumn(conflictColumnNames...), // 指定冲突的列
		DoUpdates: clause.AssignmentColumns(assignmentColumns),  // 如果冲突发生，更新的列
	}).Create(&entities).Error
	return common.SystemError(err)
}
