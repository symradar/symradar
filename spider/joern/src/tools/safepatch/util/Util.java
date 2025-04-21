package tools.safepatch.util;

/**
 * @author Eric Camellini
 *
 */
public class Util {
	
	
	public static String repeatString(String string, int length){
		StringBuffer outputBuffer = new StringBuffer(length);
		for (int i = 0; i < length; i++){
		   outputBuffer.append(string);
		}
		return outputBuffer.toString();
	}
	
//	public static boolean rangesOverlap(int x1, int x2, int y1, int y2){
//	if(x1 <= x2 && y1 <= y2)
//		return (x1 >= y1 && x1 <= y2) ||
//				(x2 >= y1 && x2 <= y2) ||
//				(y1 >= x1 && y1 <= x2) ||
//				(y2 >= x1 && y2 <= x2);
//	else
//		return false; //Malformed
//}
	
}
