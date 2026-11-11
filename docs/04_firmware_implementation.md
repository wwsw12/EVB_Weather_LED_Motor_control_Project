# 04. 펌웨어 구현과 동작 원리

> 코드에서 확인한 동작과 그 효과에 대한 해석을 구분합니다. 파일 구분은 생성 표식과 코드 위치 기준이며 실제 작성자를 보증하지 않습니다.

## 1. 파일과 호출 관계

| 파일 / 함수 | 역할 |
|---|---|
| [main.c](../Core/Src/main.c) `main()` | 초기화 후 센서 읽기, 명령 비교, PWM 변경 |
| [main.c](../Core/Src/main.c) `holding_function()` | 모터 정지 출력, 홀딩 LED 전환, `PLAY` / `END` 비교 |
| [main.c](../Core/Src/main.c) `HAL_UART_RxCpltCallback()` | 문자 버퍼 기록, 개행 처리, 1바이트 수신 재등록 |
| [main.c](../Core/Src/main.c) `__io_putchar()` | 개행 앞 CR을 추가하고 USART2로 한 바이트씩 전송 |
| [stm32f1xx_it.c](../Core/Src/stm32f1xx_it.c) | SysTick, DMA, USART2 IRQ를 HAL에 전달 |
| [stm32f1xx_hal_msp.c](../Core/Src/stm32f1xx_hal_msp.c) | 핀, 주변장치 클럭, DMA 연결, NVIC 및 리맵 |
| [syscalls.c](../Core/Src/syscalls.c) / [sysmem.c](../Core/Src/sysmem.c) | `_write`를 `__io_putchar`에 연결 / newlib heap 지원 |
| [system_stm32f1xx.c](../Core/Src/system_stm32f1xx.c), [startup](../Core/Startup/startup_stm32f103rbtx.s) | 시스템 초기화와 시작 코드 |
| [Drivers](../Drivers/) | ST HAL 및 CMSIS 구현과 장치 정의 |

## 2. ADC DMA 수집

입력은 PA1의 ADC1 IN1과 PB0의 ADC1 IN8, 출력은 `volatile uint16_t ADC_value[2]`입니다. `HAL_ADCEx_Calibration_Start()` 후 `HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&ADC_value, 2)`를 호출합니다. 변환은 연속 스캔, DMA는 halfword·메모리 증가·circular로 설정됩니다.

- [`HAL_ADCEx_Calibration_Start`](../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c)는 ADC 보정 리셋과 완료를 폴링하며 타임아웃을 검사합니다. 앱은 반환 상태를 검사하지 않습니다.
- [`HAL_ADC_Start_DMA`](../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c)는 ADC의 `DR`을 DMA 소스로 설정하고 ADC DMA 요청과 소프트웨어 변환을 시작합니다. `uint32_t*`는 HAL API 형식이며 실제 전송 폭은 MSP의 halfword 설정에 따릅니다.
- [`HAL_DMA_Start_IT`](../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c)는 ADC가 등록한 half/complete/error 콜백을 바탕으로 해당 DMA 인터럽트를 활성화합니다. IRQ는 [`DMA1_Channel1_IRQHandler`](../Core/Src/stm32f1xx_it.c) → `HAL_DMA_IRQHandler` → HAL ADC 콜백으로 이어집니다. 앱에는 ADC 완료·절반 완료 콜백 재정의가 없어 HAL의 weak 기본 구현이 사용됩니다.
- **해석**: CPU가 변환마다 직접 `DR`을 읽지 않아도 최신 버퍼를 읽는 구조입니다. 다만 길이 2의 절반/완료 IRQ는 빈번할 수 있으며, `volatile`은 두 채널의 같은 스캔 시점 스냅샷이나 중복 없는 샘플을 보장하지 않습니다.

## 3. 조도 평균과 PWM

일반 루프는 매 반복 `ADC_value[0]`을 `cds_sum`에 더하고 10회마다 정수 나눗셈으로 `cds_average`를 갱신합니다. 타이머로 샘플링을 제한하거나 DMA 완료를 기다리지 않습니다. 따라서 같은 ADC 변환 결과가 여러 번 포함될 수 있고, 평균의 시간 창은 실행 부하에 따라 달라집니다.

`rx_flag == 1`이고 `rx_buffer`가 `START`와 일치할 때의 출력 선택은 다음과 같습니다.

| 조도 평균 raw | `moter_mode` | TIM2 CH1 비교값 | TIM3 CH1 / CH2 / CH3 | 코드 주석의 색 |
|---|---:|---:|---|---|
| 1~820 | 1 | 1000 | 1000 / 0 / 0 | 파랑 |
| 821~1640 | 2 | 800 | 0 / 0 / 1000 | 초록 |
| 1641~2460 | 3 | 600 | 0 / 800 / 200 | 노랑 |
| 2461~3280 | 4 | 400 | 0 / 780 / 30 | 주황 |
| 3281~4095 | 5 | 200 | 0 / 1000 / 0 | 빨강 |

0은 어느 분기에도 속하지 않아 이전 모드와 LED 값이 유지됩니다. 모터 출력은 `moter_mode != previous_moter_mode`일 때 갱신하며, 1~5에서는 PA6을 High로 하고 TIM2 비교값을 변경합니다. PA7은 초기화·정지에서 Low로 기록되며 이 구간 분기에서 High로 설정하지 않습니다. 색과 모터 속도는 실제 회로·실측이 필요합니다. 코드의 날씨 표현은 ADC 임계 구간의 이름이며 날씨를 측정하거나 판별했다는 증거가 아닙니다.

[`HAL_TIM_PWM_Start`](../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c)는 채널과 타이머를 활성화하고, [`__HAL_TIM_SET_COMPARE`](../Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim.h)는 해당 `CCRx`에 값을 씁니다. 듀티 계산은 [설정 문서](03_cubemx_configuration.md)를 참고하세요.

## 4. X축 변화와 홀딩

`START` 분기에서 첫 X 읽기를 기준값으로 저장한 다음, 이후 반복에서 현재 `ADC_value[1]`과 직전 읽기의 **절대 raw 차이**가 500 이상이면 `holding_flag = 1` 후 `holding_function()`을 호출합니다. 이는 시간으로 정규화한 가속도 변화량이나 물리 단위 충격량이 아닙니다.

홀딩 함수는 TIM2 CH1 비교값 0, PA6/PA7 Low를 먼저 기록하고 안내 메시지를 출력합니다. 이후 `while (holding_flag == 1)`에서 버퍼를 비교하고 `HAL_GetTick() - LED_time >= 300`이면 TIM3 세 채널을 전부 1000 또는 전부 0으로 전환합니다. 이는 틱을 이용해 전환 시점을 판단하지만 **함수 자체는 바쁜 대기**이고 로그 송신도 블로킹입니다.

| 명령 | 홀딩 분기에서 수행 |
|---|---|
| `PLAY` | 홀딩 해제, X 첫 값 재설정, `previous_moter_mode=0`, LED 비교값 0, 수신 버퍼를 `START`로 변경 |
| `END` | 시작·홀딩 플래그 해제, X 첫 값 재설정, LED 비교값 0; `moter_mode`는 이 분기에서 초기화하지 않음 |

일반 `END` 분기는 모터와 LED를 끄고 모드들을 0으로 설정합니다. 홀딩 중 `END`에서 빠져나온 **같은 루프 반복**의 모터 갱신 가능성은 [미해결 항목](05_troubleshooting.md)에 명시합니다.

## 5. UART 수신 및 송신

수신 경로: `HAL_UART_Receive_IT(..., 1)` → USART2 IRQ → [`HAL_UART_IRQHandler`](../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c) → HAL 내부 `UART_Receive_IT` → `HAL_UART_RxCpltCallback()` → 다음 1바이트 수신 재등록. CR 또는 LF에 `rx_index > 0`이면 NUL을 쓰고 인덱스를 0으로 되돌립니다. 명령은 대문자 `strcmp`로 비교됩니다.

중요하게 `rx_flag=1`은 **첫 바이트부터** 설정되고 이후 초기화되지 않습니다. 따라서 개행으로 명령 완성이 확인된 경우에만 파싱하는 설계는 아닙니다. 이전 버퍼를 ISR과 메인 루프가 동시에 사용하는 위험도 있습니다.

송신 경로: `printf` → [`_write`](../Core/Src/syscalls.c) → `__io_putchar` → [`HAL_UART_Transmit`](../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c). HAL 구현은 TXE와 전송 완료 플래그를 기다리는 블로킹 방식이고, 앱은 한 문자당 timeout `0xffff`를 전달하며 반환값을 검사하지 않습니다. `__io_putchar`는 `int` 반환형이지만 return 문이 없습니다. 메시지 내용은 코드에 존재한다는 뜻이지 실제 로그를 수집했다는 뜻이 아닙니다.

## 6. 오류 처리와 한계

생성 초기화 함수는 실패 시 `Error_Handler()`에 진입합니다. 반면 앱의 보정, ADC DMA 시작, UART 수신 재등록, PWM 시작, UART 송신 반환값은 확인하지 않습니다. `Error_Handler` 역시 IRQ 비활성화 후 무한 루프이며 장치별 안전 상태 출력이나 오류 보고를 보장하지 않습니다. 수신 오류 콜백 재정의와 복구 절차도 확인되지 않았습니다.
