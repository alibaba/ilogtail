package setting

import "testing"

func TestGetSetting(t *testing.T) {
	SetSettingPath("./setting.json")
	t.Log(GetSetting())
}

func TestUpdateSetting(t *testing.T) {
	SetSettingPath("./setting.json")

	t.Log(GetSetting())

	UpdateSetting(map[string]interface{}{"store_mode": "mysql"})

	t.Log(GetSetting())

	UpdateSetting(map[string]interface{}{"store_mode": "leveldb"})

	t.Log(GetSetting())
}
