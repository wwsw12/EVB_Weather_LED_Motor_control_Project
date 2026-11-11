# 03. CubeMX와 주변장치 설정

> [.ioc](../80-8.EVB_project_dma.ioc), [초기화 코드](../Core/Src/main.c), [MSP](../Core/Src/stm32f1xx_hal_msp.c), [HAL 설정](../Core/Inc/stm32f1xx_hal_conf.h)을 대조한 명목값입니다. 실측값이 아닙니다.

## 1. 클럭

| 설정 항목 | 실제 설정값 | 설정 의미 |
|---|---|---|
| HSI / PLL | 8 MHz / 2 × 16 | SYSCLK = 64 MHz |
| AHB / APB1 / APB2 | ÷1 / ÷2 / ÷1 | HCLK 64 MHz, PCLK1 32 MHz, PCLK2 64 MHz |
| TIM2·TIM3 | APB1 timer clock 64 MHz | APB1 분주가 1이 아니므로 타이머 클럭은 PCLK1 × 2 |
| ADC | PCLK2 ÷ 8 = 8 MHz | ADC1 변환 클럭 |
| Flash | `FLASH_LATENCY_2` | 생성 코드의 지연 설정 |
| SysTick | HAL 기본 1 ms, 우선순위 15 | `SysTick_Handler()` → `HAL_IncTick()` |

HSI 8 MHz는 설정 상수이며 실제 발진 오차는 측정하지 않았습니다.

## 2. ADC1 / DMA1

| 설정 항목 | 실제 설정값 | 설정 의미 |
|---|---|---|
| ADC 순서 | rank 1 IN1(PA1), rank 2 IN8(PB0) | 버퍼 `[0]` 조도, `[1]` X 입력으로 사용 |
| 변환 | scan / continuous 활성, software start, 우측 정렬, 2회 | 시작 후 두 채널을 반복 |
| 샘플링 | 두 채널 239.5 ADC cycles | 8 MHz에서 샘플링 구간 명목 29.9375 µs |
| DMA | DMA1 Channel1, 주변장치→메모리, circular | ADC 결과를 2개 항목으로 반복 기록 |
| 데이터 폭 / 증가 | 주변장치·메모리 halfword, 메모리 증가, 주변장치 고정 | `uint16_t ADC_value[2]` 사용 |
| DMA 우선순위 / IRQ | low / NVIC 0 | DMA 중재 우선순위와 IRQ 우선순위는 서로 별개 |

12-bit ADC 변환을 12.5 cycles로 계산하면 채널당 `(239.5 + 12.5) / 8 MHz = 31.5 µs`, 두 채널 순서는 약 `63 µs`입니다. 이는 명목 이론값으로 시작·버스·인터럽트 지연과 실제 ADC 타이밍은 실측되지 않았습니다. 메인 루프의 10회 읽기는 이 10개 변환과 동의어가 아닙니다.

## 3. TIM2 / TIM3 PWM

| 설정 항목 | 실제 설정값 | 설정 의미 |
|---|---|---|
| 공통 시간 기반 | 내부 클럭 64 MHz, PSC 63, ARR 999, up-count | `64 MHz / (63 + 1) / (999 + 1) = 1 kHz`, 주기 1 ms |
| PWM | PWM mode 1, high polarity, 초기 pulse 0 | 비교값에 따른 출력 |
| TIM2 CH1 | PA0, 비교값 0/200/400/600/800/1000 | 0/20/40/60/80/100% 명목 듀티 |
| TIM3 CH1~3 | PC6/PC7/PC8, full remap | 세 LED 채널. 비교값 1000은 ARR보다 커 100%에 해당 |
| preload | auto-reload preload 활성 | ARR 갱신을 타이머 업데이트와 관련시킴 |

PWM의 실제 극성, 파형, 모터 속도 및 LED 색은 외부 회로와 실측이 필요합니다.

## 4. USART2 / GPIO / NVIC

| 설정 항목 | 실제 설정값 | 설정 의미 |
|---|---|---|
| USART2 | 115200 bps, 8 data, no parity, 1 stop, TX/RX, oversampling 16, flow control 없음 | 비동기 8N1 |
| 보율 계산 | PCLK1 32 MHz, HAL BRR 명목 `0x116` = 278 | `32,000,000 / 278 ≈ 115,108 bps`, 설정 대비 약 −0.08%; 발진 오차 제외 |
| GPIO | PA6/PA7 output push-pull, no pull, 초기 Low | 모터 제어 출력 |
| NVIC | USART2·DMA1 Channel1 우선순위 0, SysTick 15 | 두 주변장치 IRQ가 SysTick보다 높은 우선순위 |

## 5. `.ioc`와 코드 대조

| 항목 | 대조 결과 |
|---|---|
| ADC 순서·샘플링, DMA 폭·모드, PWM PSC/ARR, 핀, 클럭 | 명시된 `.ioc` 값과 초기화 코드가 일치 |
| TIM3 PC6~8 | `.ioc`에 핀 할당, MSP에 full remap 구현. 관련 CMSIS 정의와 일치 |
| USART2 세부 파라미터 | `.ioc`에는 `VirtualMode=VM_ASYNC`만 명시; 115200/8N1 등은 생성 코드에서 확인. `.ioc`의 생략을 불일치로 단정할 수 없음 |
| 보드·전압 | `.ioc`의 `custom`, `PCC.Vdd=3.3`은 실제 보드 모델이나 실측 전압을 입증하지 않음 |

코드에서 확인 가능한 설정 충돌은 위 대조 범위에서 발견되지 않았습니다. 재생성 시 기본값과 실제 물리 배선은 별도 검증이 필요합니다.
