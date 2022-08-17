package model

type Config struct {
	Name        string `json:"name"`
	Content     string `json:"content"`
	Version     int    `json:"version"`
	Description string `json:"description"`
}
