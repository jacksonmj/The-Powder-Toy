function(fuzzyoption OptionName Description DefaultValue)
	set(${OptionName} ${DefaultValue} CACHE STRING ${Description})
	set_property(CACHE ${OptionName} PROPERTY STRINGS on maybe off)
endfunction()

