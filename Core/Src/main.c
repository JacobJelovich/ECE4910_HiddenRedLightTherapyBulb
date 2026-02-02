//Includes
#include "main.h"

//Define typedef for timer settings and values
TIM_HandleTypeDef htim1;

//Define typedef for RGB values. R will be highest here to gain max therapeutic benefits
typedef struct {
  float r;
  float g;
  float b;
}LED_Recipe;

//Float combination of RGB values that will produce "Daylight-colored" light
const LED_Recipe DAYLIGHT = {1.0f, 0.82f, 0.74f};

//Variable to hold the master brightness of all LEDs. (0-1000)
uint16_t master_brightness = 1000;

//The control function that calculates the duty cycles to use in our PWM
void Apply_LED_Recipe(LED_Recipe recipe, uint16_t brightness){

  //Calculate the duty cycles 
  uint32_t red = (uint32_t)(recipe.r * brightness);
  uint32_t green = (uint32_t)(recipe.g * brightness);
  uint32_t blue = (uint32_t)(recipe.b * brightness);

  //Write to the physical PWM pins using the channels
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, red); //PA8 pin
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, green); //PA9 pin
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, blue); //PA10 pin
}


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();

  //Starting the timers for the PWMs
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

  

  //Infinite loop
  while (1)
  {
    //Use the Apply_LED_Recipe function to apply the selected recipe at the desired master brightness
    Apply_LED_Recipe(DAYLIGHT, master_brightness);
    HAL_Delay(500); //Delay 500ms for heartbeat
  }

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  //Timer Ports Clock Enable
  __HAL_RCC_TIM1_CLK_ENABLE();

}

static void MX_TIM1_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Configure pins for alternate function, push-pull(PWM Mode)
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  //Configure Timer settings
  //Select TIM1 in hardware
  htim1.Instance = TIM1;
  //Prescale value to get clock down to 1MHz (8MHz / (7+1) = 1MHz due to 0-indexing of count)
  htim1.Init.Prescaler = 7;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000; //1000 ticks = 1MHz/1000 = 1kHz
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0; 
  HAL_TIM_PWM_Init(&htim1); //Initialize timer 1 as a PWM. Pushes all setting set to htim1 to the 
  //actual internal hardware registers for timer 1.

  //Configure PWM settings
  sConfigOC.OCMode = TIM_OCMODE_PWM1; //This line ensures that the output is kept high while the
  //counter is still counting up to the pulse limit then turns off for the remaining time. This
  //relates to the brightness scaling as if brightness is 800, the timer would count to 800 while then 
  //swap to low for 200 until it hits 1000.
  sConfigOC.Pulse = 0; //Keeps lights off until the Apply_LED_Recipe function is called.
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH; //Defines that high means 3.3V and low 
  //means 0V.
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE; //Not needed for LED control.

  //Apply these settings to all three different channels, PA8 for red, PA9 for green and PA10 for blue.
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1); //Channel 1 (PA8)
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2); //Channel 2 (PA9)
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3); //Channel 3 (PA10)


}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
