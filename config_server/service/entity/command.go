package entity

type CommandInfo struct {
	Type       string
	Name       string       `gorm:"primarykey"`
	Status     ConfigStatus // Command's status
	Message    string
	Detail     []byte
	ExpireTime int64
}

func (CommandInfo) TableName() string {
	return commandInfoTable
}
