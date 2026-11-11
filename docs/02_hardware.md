# 02. 하드웨어 구성과 인터페이스

> 근거: [.ioc](../80-8.EVB_project_dma.ioc), [main.h](../Core/Inc/main.h), [MSP](../Core/Src/stm32f1xx_hal_msp.c), [main.c](../Core/Src/main.c). 프로젝트에 회로도, BOM, 데이터시트, 배선 사진은 없습니다.

## 1. 구성 요소

| 요소 | 확인된 내용 | 미확인 |
|---|---|---|
| MCU | STM32F103RBT6 / LQFP64, `.ioc`의 보드 유형은 custom | 실제 보드 모델 |
| 조도 입력 | 코드 변수 `cds`, ADC1 IN1 | 센서 모델, 분압 방향, 단위·보정 |
| 가속도 입력 | 코드 변수 `x`, ADC1 IN8 | 센서 모델, 축 방향, 영점·감도 |
| 모터와 구동부 | PWM 1선과 GPIO 2선의 제어 로직 | 모터·드라이버 종류, 배선, 정지 방식 |
| RGB LED | 세 PWM 채널과 색 이름 주석 | 실제 색 채널 연결, 공통극, 구동 회로 |
| PC 경로 | UART TX/RX와 메시지·명령 | USB-UART 변환기나 보드 연결 방식 |

## 2. MCU 핀 설정

**연결 대상은 코드가 의도한 논리적 역할이지 확인된 물리 배선이 아닙니다.**

| 신호 | 핀 | 코드 설정 / 의도된 대상 |
|---|---|---|
| ADC1 IN1 | PA1 | 아날로그 입력 / 조도 |
| ADC1 IN8 | PB0 | 아날로그 입력 / X축 |
| TIM2 CH1 | PA0 | AF push-pull, low speed / 모터 PWM |
| `motor_control1`, `motor_control2` | PA6, PA7 | GPIO push-pull, no pull, low speed, 초기 Low / 모터 제어 |
| TIM3 CH1, CH2, CH3 | PC6, PC7, PC8 | AF push-pull, low speed / LED 세 채널 |
| USART2 TX | PA2 | AF push-pull, high speed / 직렬 송신 |
| USART2 RX | PA3 | 입력, no pull / 직렬 수신 |
| SWDIO, SWCLK | PA13, PA14 | `.ioc` Serial Wire; MSP는 JTAG 비활성화, SWD 유지 |

TIM3는 MSP의 `__HAL_AFIO_REMAP_TIM3_ENABLE()`으로 full remap됩니다. [CMSIS의 정의](../Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h)는 CH1/2/3을 PC6/7/8로 명시합니다. 색별 연결은 PWM 비교값과 주석이 시사할 뿐 실제 배선 확인 전에는 확정할 수 없습니다.

## 3. 전원과 외부 부품

| 항목 | 확인 범위 |
|---|---|
| 전압 | `.ioc`의 `PCC.Vdd=3.3`, [HAL 설정](../Core/Inc/stm32f1xx_hal_conf.h)의 `VDD_VALUE=3300`은 설정·계산상의 값. 실측 공급 전압과 센서·모터 전압은 미확인 |
| ADC 신호 | 코드에 ADC 전압이나 물리 단위 환산은 없음. 실제 입력 범위와 기준 전압을 회로 및 MCU 데이터시트로 확인해야 함 |
| 저항 | CdS 분압, LED 전류 제한, 입력 풀업/풀다운 등이 실제로 있는지와 값은 미확인 |
| 커패시터 | 전원 디커플링·아날로그 필터의 장착 여부와 값은 미확인 |
| 모터 보호 | 외부 모터 드라이버, 역기전력 대책 및 전원 분리 여부 미확인 |

**일반적인 검토 사항이며 본 프로젝트 회로라고 주장하지 않음**: ADC 센서의 전압 범위 및 분압, LED 전류 제한, MCU와 드라이버의 로직 레벨·공통 기준, 모터 구동부의 보호 및 전원 디커플링을 각 부품 데이터시트와 회로도에 맞게 확인합니다. 모터를 MCU 핀에 직접 연결한다고 가정하지 않습니다.
