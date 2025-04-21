package tools.safepatch.util;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class Z3Util {

	public static final Map<String, String> limits;
	
    static {
        Map<String, String> aMap = new HashMap<String, String>();
        aMap.put("CHAR_BIT", "8");
        aMap.put("SCHAR_MIN", "-128");
        aMap.put("SCHAR_MAX", "127");
        aMap.put("UCHAR_MAX", "255");
        aMap.put("CHAR_MIN", "-128");
        aMap.put("CHAR_MAX", "127");
        aMap.put("MB_LEN_MAX", "16");
        aMap.put("SHRT_MIN", "-32768");
        aMap.put("SHRT_MAX", "32767");
        aMap.put("USHRT_MAX", "65535");
        aMap.put("INT_MIN", "-2147483648");
        aMap.put("INT_MAX", "2147483647");
        aMap.put("UINT_MAX", "4294967295");
        aMap.put("SIZE_MAX", "4294967295");
        aMap.put("LONG_MIN", "-9223372036854775808");
        aMap.put("LONG_MAX", "9223372036854775807");
        aMap.put("ULONG_MAX", "18446744073709551615");
        limits = Collections.unmodifiableMap(aMap);
    }
    
}
