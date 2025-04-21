/**
 * 
 */
package tools.safepatch;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

import tools.safepatch.cgdiff.FileDiff;

/**
 * @author machiry
 *
 */
public class FindPatchSize {

	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		// TODO Auto-generated method stub
		String srcFileName = args[0];
		String dstFileName = args[1];
		String outJsonFile = args[2];
		
		File srcFile = new File(srcFileName);
		File dstFile = new File(dstFileName);
		
		FileDiff fdiff = new FileDiff(srcFile, dstFile);
		
		FileWriter fileWriter = new FileWriter(outJsonFile);
		PrintWriter printWriter  = new PrintWriter(fileWriter);
		fdiff.diff();
		int patchSize = fdiff.getAllDeltas().size();
		
		printWriter.write("{\"patchsize\":" + Integer.toString(patchSize) + "}");
		printWriter.close();

	}

}
