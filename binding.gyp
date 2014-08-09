{
	"targets": [{
		"target_name": "tasklist",
		"sources": [ 
			"src/main.cpp"
			, "src/snapshot.cpp"
			, "src/tasklist.cpp"
		],
		"include_dirs":["src"],

		"conditions": [
			['OS=="win"', {
				"defines": [
					"UNICODE",
					"_UNICODE "
				],
				"configurations": {
					'Release': {
					  'msvs_settings': {
							'VCCLCompilerTool': {
								'WarningLevel': 4,
								'ExceptionHandling': 1,
								'DisableSpecificWarnings': [4100, 4127, 4201, 4244, 4267, 4506, 4611, 4714, 4800]
							}
					  }
					},

					'Debug': {
					  'msvs_settings': {
							'VCCLCompilerTool': {
								'WarningLevel': 4,
								'ExceptionHandling': 1,
								'DisableSpecificWarnings': [4100, 4127, 4201, 4244, 4267, 4506, 4611, 4714, 4800]
							}
					  }
					}
				}
			}]
		]
	}]
}