package main

import (
	"encoding/json"
	"fmt"

	"github.com/ghodss/yaml"
)

type Person struct {
	Name string          `json:"name"`
	Age  int             `json:"age"`
	Data json.RawMessage `json:"data"` // 原始 json 字符串
}

type LogConfig struct {
	LogLevel       string `yaml:"log_level" json:"log_level"`
	LogSize        int    `yaml:"log_size" json:"log_size"`
	LogCount       int    `yaml:"log_count" json:"log_count"`
	AccessLogSize  int64  `yaml:"access_log_size" json:"access_log_size"`
	AccessLogCount int    `yaml:"access_log_count" json:"access_log_count"`
}

type Master struct {
	ServiceName string     `yaml:"service_name" json:"service_name"`
	Version     string     `yaml:"version" json:"version"`
	LogConfig   *LogConfig `yaml:"log_config" json:"log_config"`
}

func main() {
	// 测试json反序列化
	jsonStr := `{"name": "Alice", "age": 30, "data": {"scores": [90, 80, 85], "isActive": true}}`

	var p Person
	err := json.Unmarshal([]byte(jsonStr), &p)
	if err != nil {
		fmt.Println("Error:", err)
		return
	}

	// 在这里，我们可以查看 Data 字段的内容
	fmt.Println("Raw Data:", string(p.Data))

	// 你可以根据需要进一步解析 Data
	var scoresData map[string]interface{}
	err = json.Unmarshal(p.Data, &scoresData)
	if err != nil {
		fmt.Println("Error:", err)
		return
	}

	fmt.Println("Parsed Data:", scoresData)

	// 测试YAML反序列化
	yaml_data := "log_config:\n  access_log_count: 10\n  access_log_size: 100\n  log_count: 5\n  log_level: info\n  log_size: 10\nservice_name: MyService\nversion: \"1.0.0\""

	fmt.Printf("%s\n\n\n", yaml_data)
	var master Master
	err = yaml.Unmarshal([]byte(yaml_data), &master)
	if err != nil {
		fmt.Printf("Error unmarshalling YAML: %v\n", err)
		return
	}

	// 打印解析后的数据
	fmt.Printf("Parsed YAML:\n %+v\n", master)
	fmt.Printf("Parsed YAML:\n %+v\n", *master.LogConfig)
}
