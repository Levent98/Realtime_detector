typedef struct
{
	uint16_t zero_adc_val;
	uint16_t span_adc_val;
	uint32_t zero_ratio_int;
	uint32_t span_ratio_int;
	uint8_t date[3];
	uint8_t cycle;

	uint8_t span_gas_type;
	uint8_t applied_gas;
	uint8_t dmy2;
	uint8_t dmy3;

	double zero_ratio;
	double span_ratio;
	double span;
	double span_100Vol;
	double temperature;
	double span_gas_conc;
	double aplied_gas_conc;
	double span_gas_rel_sens;
	double aplied_gas_rel_sens;
	double aplied_gas_lel_value;
}CalibrationStruct;

typedef struct
{
	double a;
	double n;
	double alpha_pos;
	double alpha_neg;
	double beta_pos;
	double beta_neg;
}CoefficientStruct;

typedef struct
{
	uint8_t gas;
	uint8_t calibration_due;
	uint8_t display_unit;
	uint8_t disable;

	uint8_t filter_update_tick;
	uint8_t reg_switched_off;
	uint8_t pellistor_saver;
	uint8_t warm_up_time;

	uint8_t bump_test_due;
	uint8_t wr_protect;
	uint8_t mul_factor;
	uint8_t supply_voltage;

	uint16_t type;
	uint16_t model;

	uint16_t man_date;
	uint16_t voc_gas;

	uint8_t bump_test_interval;
	uint8_t bump_test_date[3];

	uint16_t lamp_drv_period;
	uint8_t rd_time_delay_peak;
	uint8_t rd_time_delay_deep;

	uint16_t th_resistor;
	uint16_t th_beta;

	uint16_t call_span_time;
	uint16_t call_zero_time;

	uint8_t serial_number[12];
	uint8_t man_id_number[12];

	double th_coeff;
	double gas_conc_range;
	double gas_lel_value;
	double lel_correction_fact;
	double voc_response_fact;
	double relative_sensitivity;
	double opr_temp_max;
	double opr_temp_min;
	double upper_range_limit;
	double lower_range_limit;
	double over_range_reset_thr;

	CoefficientStruct Coefficient;
	CalibrationStruct Calibration;
}SensorStruct;