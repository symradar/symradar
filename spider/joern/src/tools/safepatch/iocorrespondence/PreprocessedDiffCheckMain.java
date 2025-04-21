/**
 * 
 */
package tools.safepatch.iocorrespondence;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.sun.org.apache.xpath.internal.operations.Bool;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchMain;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.flowres.FlowRestrictiveChecker.SATISFIABLE_FLAG;
import tools.safepatch.flowres.FunctionMap;
import tools.safepatch.flowres.FunctionMatcher;
import tools.safepatch.flowres.FunctionMatcher.MATCHING_METHOD;

/**
 * @author machiry
 *
 */
public class PreprocessedDiffCheckMain {
	
	private static final Logger logger = LogManager.getLogger();
	private static boolean removePreProcess = true;

	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		org.apache.logging.log4j.core.config.Configurator.setRootLevel(Level.DEBUG);
		String srcFileName = args[0];
		String outJsonFile = args[1];
		
		File srcFile = new File(srcFileName);
		File srcPreprocessFile = new File(srcFileName + ".preprocess");
		PrintWriter printWriter = null;
		
		try {
			
			FileWriter fileWriter = new FileWriter(outJsonFile);
		    printWriter = new PrintWriter(fileWriter);
		    
			// pre-process the files.
			if(PreprocessedDiffCheckMain.preprocessFile(srcFile, srcPreprocessFile)) {
				logger.info("Preprocessed the src file:{} to {}", srcFile.getAbsolutePath(), srcPreprocessFile.getAbsolutePath());
			}
			
			String srcPreProPath = srcFile.getAbsolutePath();
			String dstPreProPath = srcPreprocessFile.getAbsolutePath();
			parser.addObserver(astWalker);
			parser.parseFile(srcPreProPath);
			parser.parseFile(dstPreProPath);
			
			//After parsing a file the SafepatchASTWalker saves a list of the function AST nodes in a Map,
			//for every parsed file.
			ArrayList<FunctionDef> oldFunctionList = astWalker.getFileFunctionList().get(srcPreProPath);
			ArrayList<FunctionDef> newFunctionList = astWalker.getFileFunctionList().get(dstPreProPath);
			String diffAnalyzableReason = isDiffAnalyzable(oldFunctionList, newFunctionList);

			// sanity check
			printWriter.write("{\"result\":{");
			if(diffAnalyzableReason != null) {
				logger.error("Give files are not analyzable by the current system.");
				//printWriter.write("\"nonanalyzable\": \"" + diffAnalyzableReason + "\"");
				
				
				
			} 
				// ok the files are analyzable.
				// go-on and do the stuff.
				FunctionMatcher currMatcher = new FunctionMatcher(oldFunctionList, newFunctionList, MATCHING_METHOD.NAME_ONLY);
				currMatcher.match();
				
				List<FunctionMap> allFunctionMap = currMatcher.getCommonFunctions();
				
				// now process the functions that changed.
				for(FunctionMap currMap:allFunctionMap) {
					if(!currMap.processFunctions()) {
						logger.error("Error occured while processing function diff");
					}
				}
				
				// OK, now get only those functions that differ.
				List<FunctionMap> diffFunctions = new ArrayList<FunctionMap>();
				for(FunctionMap fmap: allFunctionMap) {
					if(!fmap.areSame()) {
						diffFunctions.add(fmap);
					}
				}
				
				
				if(diffFunctions.size() == 0) {
					logger.info("No changed functions detected. May be there changes are in other locations");
					printWriter.write("\"nonanalyzable\": \"nonfuncchanges\"");
				} else {
					logger.info("Found {} functions changed between old and new file", diffFunctions.size());
					if(diffFunctions.size() != 1) {
						logger.info("Multiple functions ({}) are modified. We do not handle this currently", diffFunctions.size());
						// ensure that all the modified functions are independent.
						logger.debug("MULTIPLE FUNCTIONS");
					}
					
					printWriter.write("\"numfunctions\": " + Integer.toString(diffFunctions.size()) + ", "); 
					
					boolean all_fine = true;
						// TODO: handle the stuff.
					printWriter.write("\"preprocesseddiff\": [");
					boolean addComma = false;
					for(FunctionMap fmap:diffFunctions) {
						
						if(addComma) {
							printWriter.write(",");
						}
						printWriter.write("\"");
						printWriter.write(fmap.getOldFunction().getLabel());
						printWriter.write("\"");
						addComma = true;
					}
					printWriter.write("]");
					if(diffAnalyzableReason != null) {
						printWriter.write("\"oldunmatched\":[");
						addComma = false;
						for(FunctionDef cd:oldFunctionList) {
							if(!currMatcher.isOldFuncMatched(cd)) {
								if(addComma) {
									printWriter.write(",");
								}
								addComma = true;
								printWriter.write("\"" + cd.getLabel() + "\"");
							}
						}
						printWriter.write("],");
						
						printWriter.write("\"newunmatched\":[");
						addComma = false;
						for(FunctionDef cd:newFunctionList) {
							if(!currMatcher.isNewFuncMatched(cd)) {
								if(addComma) {
									printWriter.write(",");
								}
								addComma = true;
								printWriter.write("\"" + cd.getLabel() + "\"");
							}
						}
						printWriter.write("],");
						
						
					}
					if(all_fine) {
						logger.debug("ALL GOOD MACHIRY");
					} else {
						logger.debug("SOME BAD MACHIRY");
					}
									
				}
			printWriter.write("\n}}");
			
			
		} catch(Exception e) {
			e.printStackTrace();
			logger.error("Error occured in main function: {}", e.getMessage());
		} finally {
			// clean up
			if(PreprocessedDiffCheckMain.removePreProcess) {
				if(srcPreprocessFile.isFile()) {
					srcPreprocessFile.delete();
				}
			}
			if(printWriter != null) {
				printWriter.close();
			}
		}

	}
	
	// private methods
	
	/***
	 * 
	 * @param oldFunctions
	 * @param newFunctions
	 * @return
	 */
	private static String isDiffAnalyzable(ArrayList<FunctionDef> oldFunctions, ArrayList<FunctionDef> newFunctions) {
		boolean retVal = true;
		if(SafepatchMain.containsDuplicates(oldFunctions) || SafepatchMain.containsDuplicates(newFunctions)) {
			return "CONDITIONAL COMPILATION";
		} else if(SafepatchMain.functionRenamed(oldFunctions, newFunctions)) {
			return "FUNCTIONS RENAMED";
		} else {
			retVal = oldFunctions.size() == newFunctions.size();
			if(!retVal) {
				return "FUNCTIONS ADDED OR REMOVED";
			}
		}
		return null;
	}
	
	
	/***
	 * 
	 * @param srcFile
	 * @param dstFile
	 * @return
	 */
	private static boolean preprocessFile(File srcFile, File dstFile) {
		boolean retVal = true;
		//TODO: finish this.
		try {
			SafepatchPreprocessor prep = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			prep.preProcess(srcFile.getAbsolutePath(), dstFile.getAbsolutePath());
		} catch(Exception e) {
			logger.error(e.getMessage());
			retVal = false;
		}
		return retVal;
	}

}
