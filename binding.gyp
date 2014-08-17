{
	"targets": [{
		"target_name": "process_list",
		"sources": [ 
			"src/main.cpp"
			, "src/snapshot.cpp"
		],
		"include_dirs":["src"],

		"conditions": [
			['OS=="win"', {
				"defines": [
					"UNICODE",
					"_UNICODE "
				],
				"sources": [
					"src/win/win_tasklist.cpp"
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
			},{	# OS != win
				"sources": [
					"src/unix/tasklist.cpp"
				],
				'cflags_cc!': ['-fno-rtti'],
				'cflags_cc+': ['-frtti'],
				'cflags_cc!': ['-fno-exceptions'],
				"cflags_cc+": [
					"-fexceptions",
					"-std=c++0x"
				]
			}],
			['OS=="mac"', {
			    'xcode_settings': { 'GCC_ENABLE_CPP_RTTI': 'YES' }
			}]
		]
	}]
}