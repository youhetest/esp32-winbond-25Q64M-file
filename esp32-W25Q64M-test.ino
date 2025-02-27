#include <SPI.h>

// W25Q64 配置
#define FLASH_CS_PIN    5       // ESP32 的 CS 引脚
#define FLASH_SIZE      0x800000 // 8MB (W25Q64)
#define SECTOR_SIZE     4096    // 扇区大小 4KB
#define TEST_ADDR       0x1000  // 测试起始地址（需4KB对齐）

SPIClass hspi(HSPI); // 使用 ESP32 的 HSPI 总线

// W25Q64 指令集
#define CMD_WRITE_ENABLE    0x06
#define CMD_READ_DATA       0x03
#define CMD_PAGE_PROGRAM    0x02
#define CMD_SECTOR_ERASE    0x20
#define CMD_READ_STATUS     0x05

// 初始化 SPI
void flash_init() {
  hspi.begin(18, 19, 23, FLASH_CS_PIN); // CLK,MISO,MOSI,CS
  pinMode(FLASH_CS_PIN, OUTPUT);
  digitalWrite(FLASH_CS_PIN, HIGH);
}

// 发送指令并读取状态
uint8_t flash_read_status() {
  digitalWrite(FLASH_CS_PIN, LOW);
  hspi.transfer(CMD_READ_STATUS);
  uint8_t status = hspi.transfer(0x00);
  digitalWrite(FLASH_CS_PIN, HIGH);
  return status;
}

// 等待操作完成
void flash_wait_busy() {
  while (flash_read_status() & 0x01); // 检查 BUSY 位
}

// 擦除扇区（地址需4KB对齐）
void flash_erase_sector(uint32_t addr) {
  digitalWrite(FLASH_CS_PIN, LOW);
  hspi.transfer(CMD_WRITE_ENABLE);
  digitalWrite(FLASH_CS_PIN, HIGH);

  digitalWrite(FLASH_CS_PIN, LOW);
  hspi.transfer(CMD_SECTOR_ERASE);
  hspi.transfer(addr >> 16);    // 地址高位
  hspi.transfer(addr >> 8);     // 地址中位
  hspi.transfer(addr);          // 地址低位
  digitalWrite(FLASH_CS_PIN, HIGH);
  
  flash_wait_busy();
}

// 写入数据（最大256字节/页）
void flash_write_data(uint32_t addr, uint8_t *data, uint16_t len) {
  digitalWrite(FLASH_CS_PIN, LOW);
  hspi.transfer(CMD_WRITE_ENABLE);
  digitalWrite(FLASH_CS_PIN, HIGH);

  digitalWrite(FLASH_CS_PIN, LOW);
  hspi.transfer(CMD_PAGE_PROGRAM);
  hspi.transfer(addr >> 16);
  hspi.transfer(addr >> 8);
  hspi.transfer(addr);
  for (uint16_t i=0; i<len; i++) {
    hspi.transfer(data[i]);
  }
  digitalWrite(FLASH_CS_PIN, HIGH);
  
  flash_wait_busy();
}

// 读取数据
void flash_read_data(uint32_t addr, uint8_t *buf, uint16_t len) {
  digitalWrite(FLASH_CS_PIN, LOW);
  hspi.transfer(CMD_READ_DATA);
  hspi.transfer(addr >> 16);
  hspi.transfer(addr >> 8);
  hspi.transfer(addr);
  for (uint16_t i=0; i<len; i++) {
    buf[i] = hspi.transfer(0x00);
  }
  digitalWrite(FLASH_CS_PIN, HIGH);
}

void setup() {
  Serial.begin(115200);
  flash_init();

  // 测试文本
  const char* text = "Hello from W25Q64! ESP32 Storage Test.";
  uint8_t buffer[256];
  memset(buffer, 0xFF, sizeof(buffer)); // 初始化为0xFF
  memcpy(buffer, text, strlen(text));

  // 写入操作
  flash_erase_sector(TEST_ADDR);        // 必须擦除扇区
  flash_write_data(TEST_ADDR, buffer, sizeof(buffer));
  Serial.println("Text written to NorFlash.");

  // 读取并打印
  uint8_t readBuffer[256];
  flash_read_data(TEST_ADDR, readBuffer, sizeof(readBuffer));
  Serial.print("Read from NorFlash: ");
  Serial.println((char*)readBuffer);
}

void loop() {}