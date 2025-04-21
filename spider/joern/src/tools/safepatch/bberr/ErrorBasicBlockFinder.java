/**
 * 
 */
package tools.safepatch.bberr;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import ast.functionDef.FunctionDef;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.*;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;

/**
 * @author machiry
 *
 */
public class ErrorBasicBlockFinder {
	
	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();

	/**
	 * The main function that handles
	 * identifying error basic blocks in the CFGs of all functions.
	 * @param args path to the .c file.
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		String srcFileName = args[0];
		float coveragethreshold = Float.parseFloat(args[1]);
		
		ErrorBBDetector.ERRORTHRESHOLD = coveragethreshold;
		
		File givenFile = new File(srcFileName);
		File preProcessedFile = new File(srcFileName + ".preprocess");
		
		String fileAbsPath = givenFile.getAbsolutePath();
		String ppAbsPath = preProcessedFile.getAbsolutePath();
		
		// preprocess.
		SafepatchPreprocessor preProcessor = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
		preProcessor.preProcess(fileAbsPath, ppAbsPath);
		
		// parse the provided file to get all the function defs.
		parser.addObserver(astWalker);
		parser.parseFile(ppAbsPath);
		
		String retVal = "{\"errfinder\":[";
		
		boolean needComma = false;
		ArrayList<FunctionDef> allFuncDefs = astWalker.getFileFunctionList().get(ppAbsPath);
		for(FunctionDef currdef:allFuncDefs) {
			if(needComma) {
				retVal += ",\n";
			}
			ErrorBBDetector currDetctor = new ErrorBBDetector(currdef);
			retVal += currDetctor.identifyAllErrorBBs();
			needComma = true;
		}
		
		retVal += "\n]}";
		
		System.out.println(retVal);
		
	}

}
