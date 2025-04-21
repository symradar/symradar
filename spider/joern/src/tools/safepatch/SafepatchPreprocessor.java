package tools.safepatch;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * @author Eric Camellini
 * 
 */
public class SafepatchPreprocessor {

	public enum PREPROCESSING_METHOD{
		/*
		 * No preprocessing, the file is rewritten as it is
		 */
		NO_PREPROCESSING,
		
		/*
		 * This method removes all the preprocessor macros in order to
		 * have all the code in the analyzed file.
		 * (Since Joern automatically considers only the #ifdef part)
		 */
		CONSIDER_ALL_BRANCHES,
		
		/*
		 * http://dotat.at/prog/unifdef/
		 */
		UNIFDEFALL
	}
	
	private PREPROCESSING_METHOD method;
	private Charset charset = StandardCharsets.UTF_8;
	
	public SafepatchPreprocessor(PREPROCESSING_METHOD method) {
		super();
		this.method = method;
	}

	/**
	 * Applies the chosen preprocessing method to the file addressed by
	 * filePath and writes the result in the outputFilePath
	 */
	public void preProcess(String filePath, String outputFilePath) throws IOException{
		
		switch (this.method) {
		case CONSIDER_ALL_BRANCHES:
			this.considerAll(filePath, outputFilePath);
			break;

		case NO_PREPROCESSING:
			//Just writing the same stuff in the preprocessed file
			Files.write(Paths.get(outputFilePath), Files.readAllBytes(Paths.get(filePath)));
			break;
		
		case UNIFDEFALL:
			try {
				this.unifdefAll(filePath, outputFilePath);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			break;
			
		default:
			break;
		}
		
	}
	
	@Deprecated
	private void considerAll(String filePath, String outputFilePath) throws IOException{
		/*TODO: 
		 * - This does match stuff inside constant strings;
		 * - Should we delete also the other kinds of macros (#define, #include, etc.)?;
		 */

		String content = new String(Files.readAllBytes(Paths.get(filePath)), this.charset);
		content = content.replaceAll("((#if|#ifdef|#ifndef|#else|#elif|#endif)+[^\r\n]*+\r?+\n)", "//$1");
		
		Files.write(Paths.get(outputFilePath), content.getBytes(this.charset));
	}
	
	private void unifdefAll(String filePath, String outputFilePath) throws InterruptedException, IOException{
		/*
		 * TODO:
		 * - Understand in a better way if this tool works as expected
		 */
		Process p;
		String command = "unifdefall " + filePath.toString(); //+ " > " + outputFilePath.toString();
		p = Runtime.getRuntime().exec(command);
		//p.waitFor();
		
		InputStream in = new BufferedInputStream( p.getInputStream());
		OutputStream out = new BufferedOutputStream( new FileOutputStream( outputFilePath ));

		int cnt;
		byte[] buffer = new byte[1024];
		while ( (cnt = in.read(buffer)) != -1) {
		   out.write(buffer, 0, cnt );
		}
		out.close();
	}
	
	public static void main(String[] args){
		try {
			SafepatchPreprocessor s = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			s.preProcess("../test_files/preprocessor_test_case.c","../test_files/tmp.c");
			//Files.delete(Paths.get("../test_files/tmp.c"));
		} catch (IOException e) {
			e.printStackTrace();
		}	
	}
	
}
