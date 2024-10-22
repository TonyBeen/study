package main

import (
	"encoding/json"
	"fmt"

	"github.com/ghodss/yaml"
)

type Person struct {
	Name string          `yaml:"name" json:"name"`
	Age  int             `yaml:"age" json:"age"`
	Data json.RawMessage `yaml:"data" json:"data"` // 原始 json 字符串
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
	Msg         []string   `yaml:"msg,omitempty" json:"msg,omitempty"` // omitempty 字段的作用是当不存在此键时忽略, 并且在序列化的时候如果为nil也不对其进行序列化
}

func test_person() {
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
}

func test_yaml_master() {
	// 测试YAML反序列化
	yaml_data := "log_config:\n  access_log_count: 10\n  access_log_size: 100\n  log_count: 5\n  log_level: info\n  log_size: 10\nservice_name: MyService\nversion: \"1.0.0\""

	fmt.Printf("%s\n\n\n", yaml_data)
	var master Master
	err := yaml.Unmarshal([]byte(yaml_data), &master)
	if err != nil {
		fmt.Printf("Error unmarshalling YAML: %v\n", err)
		return
	}

	// 打印解析后的数据
	fmt.Printf("Parsed YAML:\n %+v\n", master)
	fmt.Printf("Parsed YAML:\n %+v\n", *master.LogConfig)
}

func test_json_master() {
	// 测试 json 反序列化
	json_data := `{
		"service_name": "MyService",
		"version": "1.0.0",
		"log_config":{"level":"INFO"},
		"persons": []
	}`

	fmt.Printf("%s\n\n\n", json_data)
	var master Master = Master{
		Msg: []string{},
	}

	err := json.Unmarshal([]byte(json_data), &master)
	if err != nil {
		fmt.Printf("Error unmarshalling YAML: %v\n", err)
		return
	}

	master_have_msg := master
	master_have_msg.Msg = append(master_have_msg.Msg, "Hello", "World")

	// 打印解析后的数据
	fmt.Printf("Parsed JSON:\n %+v\n", master)
	fmt.Printf("Parsed JSON:\n %+v\n", *master.LogConfig)

	// 序列化
	json_byte_array, err := json.Marshal(master)
	// 由于设置了 omitempty 字段, 故空数组不会被序列化, 即看不到msg字段
	fmt.Printf("Marshal JSON:\n %s\n", json_byte_array)

	json_byte_array, err = json.Marshal(master_have_msg)
	// 此时可正常看到 msg
	fmt.Printf("\nMarshal JSON:\n %s\n", json_byte_array)
}

func main() {
	test_person()
	test_yaml_master()
	test_json_master()
}
