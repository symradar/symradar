/**
 * 
 */
package tools.safepatch.iocorrespondence;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.cfgclassic.ClassicCFG;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.flowres.FlowRestrictiveMainConfig;
import tools.safepatch.flowres.FunctionMap;
import tools.safepatch.flowres.FunctionMatcher;
import tools.safepatch.flowres.FunctionMatcher.MATCHING_METHOD;

/**
 * @author machiry
 *
 */
public class StatementLoopChecker {
	
	private static final Logger logger = LogManager.getLogger();
	private static boolean removePreProcess = true;

	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		org.apache.logging.log4j.core.config.Configurator.setRootLevel(Level.DEBUG);
		// get the args.
		String srcFileName = args[0];
		String dstFileName = args[1];
		String outJsonFile = args[2];
		boolean set_op = false;
		if(!set_op) {
			logger.info("Enabling error bb heuristics");
			FlowRestrictiveMainConfig.ignoreErrorBB = false;
		}
		
		File srcFile = new File(srcFileName);
		File dstFile = new File(dstFileName);
		File srcPreprocessFile = new File(srcFileName + ".preprocess");
		File dstPreprocessFile = new File(dstFileName + ".preprocess");
		PrintWriter printWriter = null;
		
		try {
			printWriter = new PrintWriter(new FileWriter(outJsonFile));;
			
			// pre-process the input files.
			if(IOCorrespondenceChecker.preprocessFile(srcFile, srcPreprocessFile)) {
				logger.info("Preprocessed the src file:{} to {}", srcFile.getAbsolutePath(), srcPreprocessFile.getAbsolutePath());
			}
			
			if(IOCorrespondenceChecker.preprocessFile(dstFile, dstPreprocessFile)) {
				logger.info("Preprocessed the dst file:{} to {}", dstFile.getAbsolutePath(), dstPreprocessFile.getAbsolutePath());
			}
			
			
			String srcPreProPath = srcPreprocessFile.getAbsolutePath();
			String dstPreProPath = dstPreprocessFile.getAbsolutePath();
			parser.addObserver(astWalker);
			parser.parseFile(srcPreProPath);
			parser.parseFile(dstPreProPath);
			
			//After parsing a file the SafepatchASTWalker saves a list of the function AST nodes in a Map,
			//for every parsed file.
			ArrayList<FunctionDef> oldFunctionList = astWalker.getFileFunctionList().get(srcPreProPath);
			ArrayList<FunctionDef> newFunctionList = astWalker.getFileFunctionList().get(dstPreProPath);
			
			printWriter.write("{\"result\":{");
			printWriter.write("\"spider\":{");
			long startTime = System.currentTimeMillis();
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

			printWriter.write("\"numfunctions\": " + Integer.toString(diffFunctions.size()) + ", "); 

			boolean all_fine = true;

			// check if the changes between the different functions are safe or not.
			printWriter.write("\"info\": [");
			boolean addComma = false;
			for(FunctionMap fmap:diffFunctions) {

				if(addComma) {
					printWriter.write(",");
				}
				printWriter.write("{");
				if(StatementLoopChecker.checkIfAffectedStatementNotInLoop(fmap, printWriter)) {
					logger.debug("VOK TO FUNCTION: {} are safe", fmap.getOldFunction().getLabel());
				} else {
					logger.debug("NOTOK TO FUNCTION: {} are NOT safe", fmap.getOldFunction().getFunctionSignature());
					all_fine = false;
				}
				printWriter.write("}");
				addComma = true;
			}
			printWriter.write("]");
			if(all_fine) {
				logger.debug("ALL GOOD MACHIRY");
			} else {
				logger.debug("SOME BAD MACHIRY");
			}
			
			printWriter.write("},");
			printWriter.write("\"totaltime\":");
			long totalTime = System.currentTimeMillis() - startTime;
			printWriter.write(Long.toString(totalTime));			
			printWriter.write("\n}}");
			
			
		} catch(Exception e) {
			e.printStackTrace();
			logger.error("Error occured in main function: {}", e.getMessage());
		} finally {
			// clean up
			if(StatementLoopChecker.removePreProcess) {
				if(srcPreprocessFile.isFile()) {
					srcPreprocessFile.delete();
				}
				if(dstPreprocessFile.isFile()) {
					dstPreprocessFile.delete();
				}
			}
			if(printWriter != null) {
				printWriter.close();
			}
		}

	}
	
	private static boolean checkCyclicReachability(BasicBlock currBB, HashSet<BasicBlock> visitedBBs) {
		if(visitedBBs.contains(currBB)) {
			return true;
		}
		boolean retVal = false;
		visitedBBs.add(currBB);
		for(BasicBlock currCH: currBB.getAllChildren()) {
			if(checkCyclicReachability(currCH, visitedBBs)) {
				retVal = true;
				break;
			}
		}
		visitedBBs.remove(currBB);
		return retVal;
	}
	
	
	private static boolean isStatementInLoop(ASTNode currSt, ClassicCFG targetCFG) {
		boolean retVal = false;
		if(currSt != null && 
		   targetCFG.getBB(currSt) != null && 
		   !targetCFG.getBB(currSt).isTrueErrorHandling()) {
			retVal = checkCyclicReachability(targetCFG.getBB(currSt), new HashSet<BasicBlock>());
		}
		return retVal;
	}
	
	private static boolean checkIfAffectedStatementNotInLoop(FunctionMap currFMap, PrintWriter targetWrite) {
		boolean retVal = true;
		
		for(StatementMap currStMap: currFMap.getStatementMap()) {
			ASTNode oldNode = currStMap.getOriginal();			
			ASTNode newNode = currStMap.getNew();
			if(isStatementInLoop(oldNode, currFMap.getOldClassicCFG()) || 
			   isStatementInLoop(newNode, currFMap.getNewClassicCFG())) {
				retVal = false;
				break;
			}
		}
		targetWrite.write("\"fun\":\""+ currFMap.getOldFunction().getLabel() + "\", \"hasinLoop\":" + retVal);
		
		return retVal;
	}

}
