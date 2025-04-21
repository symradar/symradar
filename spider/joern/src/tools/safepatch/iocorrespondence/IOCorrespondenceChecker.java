/**
 * 
 */
package tools.safepatch.iocorrespondence;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.expressions.CallExpression;
import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import ast.statements.Statement;
import cdg.DominatorTree;
import cfg.nodes.CFGNode;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchMain;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.flowres.DeletedConditionChecker;
import tools.safepatch.flowres.FlowRestrictiveMainConfig;
import tools.safepatch.flowres.FunctionMap;
import tools.safepatch.flowres.FunctionMatcher;
import tools.safepatch.flowres.FunctionMatcher.MATCHING_METHOD;
import tools.safepatch.flowres.MovedStatementChecker;
import tools.safepatch.heuristics.CodeHeuristics;
import tools.safepatch.iocorrespondence.FlowRestrictiveChecker;
import tools.safepatch.iocorrespondence.FlowRestrictiveChecker.SATISFIABLE_FLAG;

/**
 * Main class that checks the output correspondence
 * of all the modified functions.
 * 
 * @author machiry
 *
 */
public class IOCorrespondenceChecker {
	
	private static final Logger logger = LogManager.getLogger();
	private static boolean removePreProcess = true;

	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();
	private static HashSet<ASTNode> oldFuncErrorNodes = new HashSet<ASTNode>();
	private static HashSet<ASTNode> newFuncErrorNodes = new HashSet<ASTNode>();

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
		if(args.length > 3) {
			String tarOp = args[3];
			if(tarOp.contains("error")) {
				logger.info("Ignoring error bb heuristics");
				FlowRestrictiveMainConfig.ignoreErrorBB = true;
				set_op = true;
			}
		}
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
			String diffAnalyzableReason = isDiffAnalyzable(oldFunctionList, newFunctionList);
			
			printWriter.write("{\"result\":{");
			printWriter.write("\"spider\":{");
			long startTime = System.currentTimeMillis();
			if(diffAnalyzableReason != null) {
				logger.error("Give files are not analyzable by the current system.");
				printWriter.write("\"nonanalyzable\": \"" + diffAnalyzableReason + "\"");
			} else {
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
						if(IOCorrespondenceChecker.isPatchSafe(fmap, printWriter)) {
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
									
				}
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
			if(IOCorrespondenceChecker.removePreProcess) {
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
	
	
	
	/***
	 *  Given a function map, checks whether the changes between them are safe or not.
	 *  
	 * @param targetMap The map of old function and the new function.
	 * @param jsonWriter Json writer to which the output should be written to.
	 * @return true if safe else false.
	 */
	private static boolean isPatchSafe(FunctionMap targetMap, PrintWriter jsonWriter) {
		boolean retVal = true;
		oldFuncErrorNodes.clear();
		newFuncErrorNodes.clear();
		jsonWriter.write("\"funcName\": \"" + targetMap.getOldFunction().getLabel() + "\",");
		
		ASTNode.enableHashCode = true;
		// compute mapped conditions.
		targetMap.computeMappedConditions();
		
		FlowRestrictiveChecker currChecker = new FlowRestrictiveChecker(targetMap);
		// get the conditions to check
		currChecker.computeConditionsToCheck();		
		
		currChecker.computeConditionTransDependency();
		
		currChecker.computeTransitivelyAffectedStatements();
		
		if(currChecker.hasDeletedConditions()) {
			DeletedConditionChecker delCondChecked = new DeletedConditionChecker(targetMap);
			if(delCondChecked.areDeletedConditionsSafe(jsonWriter, targetMap.getOldFnConv(), targetMap.getNewFnConv(), targetMap.getZ3Ctx())) {
				jsonWriter.write("\"DeletedStatementsResult\":" + true + ",");
			} else {
				jsonWriter.write("\"DeletedStatementsResult\":" + false + ",");
				retVal = false;
			}
		} else {
			jsonWriter.write("\"deletedConditions\":\"None\"" + ",");
		}
		
		if(retVal) {
			HashMap<ASTNode, ASTNode> conditionsToCheck = currChecker.getConditionsToCheck();
	
			if(conditionsToCheck.isEmpty()) {			
				jsonWriter.write("\"affectedConditions\":\"None\"" + ",");
			} else {
				jsonWriter.write("\"affectedConditions\":" + conditionsToCheck.size() + ",");
			}
	
			for(ASTNode oldCondNode:conditionsToCheck.keySet()) {
				ASTNode newCondNode = conditionsToCheck.get(oldCondNode);
	
				BasicBlock oldBB = targetMap.getOldClassicCFG().getBB(oldCondNode);
				BasicBlock newBB = targetMap.getNewClassicCFG().getBB(newCondNode);
	
				SATISFIABLE_FLAG satisFla = SATISFIABLE_FLAG.NEW_IMPL_OLD;
				if(newBB.isNegativeErrorGuarding()) {
					// error handling
					satisFla = SATISFIABLE_FLAG.OLD_IMPL_NEW;
				}
				if(!currChecker.checkConditionImplication(oldCondNode, newCondNode, satisFla)) {
					logger.debug("Implication Mismatched: Old condition: {}, new condition: {}, expected implication: {}\n", oldCondNode.getEscapedCodeStr(), newCondNode.getEscapedCodeStr(), satisFla);
					retVal = false;
				} else {
					logger.debug("Implication Matched: Old condition: {}, new condition: {}, expected implication: {}\n", oldCondNode.getEscapedCodeStr(), newCondNode.getEscapedCodeStr(), satisFla);
				}
			}
	
			jsonWriter.write("\"condResult\":" + Boolean.toString(retVal) + ",");
		}

		if(retVal) {				
			// check updated statements.
			boolean updatedStResult = false;
			List<StatementMap> updatedStatements = getTargetStatements(targetMap, currChecker, MAP_TYPE.UPDATE);
			if(!updatedStatements.isEmpty()) {
				updatedStResult = true;
				jsonWriter.write("\"updatedStatements\":" + updatedStatements.size() + ",");
				for(StatementMap currSt: updatedStatements) {
					if(!currChecker.checkUpdatedStatementEquivalence(currSt)) {
						logger.debug("Updated statement equivalence check failed, old st:{}, new st:{}", currSt.getOriginal().getEscapedCodeStr(), currSt.getNew().getEscapedCodeStr());
						updatedStResult = false;
						break;
					}
				}
			} else {
				jsonWriter.write("\"updatedStatements\":\"None\"" + ",");
				updatedStResult = true;
			}
			jsonWriter.write("\"UpdatedStatementsResult\":" + updatedStResult + ",");

			if(!updatedStResult) {
				retVal = false;
			}
		}

		if(retVal) {
			// check transitive affected statements.
			// these are the statements affected by the patch but not directly.
			HashSet<StatementMap> transAffectedStmnts = currChecker.getTransitivelyAffectedStatements();
			boolean transAffectedStResult = false;
			if(!transAffectedStmnts.isEmpty()) {
				transAffectedStResult = true;
				jsonWriter.write("\"transStatements\":" + transAffectedStmnts.size() + ",");
				for(StatementMap currSt: transAffectedStmnts) {
					if(!currChecker.checkUpdatedStatementEquivalence(currSt)) {
						logger.debug("Transitively affected statement equivalence check failed, old st:{}, new st:{}", currSt.getOriginal().getEscapedCodeStr(), currSt.getNew().getEscapedCodeStr());
						transAffectedStResult = false;
						break;
					}
				}
			} else {
				jsonWriter.write("\"transStatements\":\"None\"" + ",");
				transAffectedStResult = true;
			}
			jsonWriter.write("\"TransStatementsResult\":" + transAffectedStResult + ",");

			if(!transAffectedStResult) {
				retVal = false;
			}

		}

		if(retVal) {
			// check inserted statements.
			boolean insertedStResult = false;
			List<Statement> newInsertedStatements = new ArrayList<Statement>();
			List<StatementMap> insertedStatements = getTargetStatements(targetMap, currChecker, MAP_TYPE.INSERT);
			if(!insertedStatements.isEmpty()) {
				insertedStResult = true;
				CodeHeuristics targetHeuristics = new CodeHeuristics();
				for(StatementMap currSt: insertedStatements) {
					ASTNode insertedSt = currSt.getNew();
					if(insertedSt instanceof Statement && 
							!targetHeuristics.isInitNew((Statement)insertedSt)) {
						logger.debug("Inserted statement {} is not an init statement", insertedSt.getEscapedCodeStr());
						insertedStResult = false;
						break;
					} else {
						if(insertedSt instanceof Statement) {
							newInsertedStatements.add((Statement)insertedSt);
						}
						logger.debug("Inserted Statement {} is an init statement",insertedSt.getEscapedCodeStr());						
					}
				}
				jsonWriter.write("\"insertedStatements\":" + insertedStatements.size() + ",");

			} else {
				jsonWriter.write("\"insertedStatements\":\"None\"" + ",");
				insertedStResult = true;
			}
			jsonWriter.write("\"InsertedStatementsResult\":" + insertedStResult + ",");

			if(!insertedStResult) {
				retVal = false;
			} else {
				InsertedFunctionChecker newF = new InsertedFunctionChecker(targetMap);
				if(!newF.checkInsertedFunctionSanity(newInsertedStatements)) {
					retVal = false;
					jsonWriter.write("\"LockResult\":" + retVal + ",");
				}
			}
		}

		if(retVal) {
			// check deleted statements.
			boolean deletedStResult = false;
			List<StatementMap> deletedStatements = getTargetStatements(targetMap, currChecker, MAP_TYPE.DELETE);
			if(!deletedStatements.isEmpty()) {
				deletedStResult = true;
				CodeHeuristics targetHeuristics = new CodeHeuristics();
				for(StatementMap currSt: deletedStatements) {
					ASTNode insertedSt = currSt.getOriginal();
					if(insertedSt instanceof Statement && 
							!targetHeuristics.isInitNew((Statement)insertedSt)) {
						logger.debug("Deleted statement {} is not an init statement", insertedSt.getEscapedCodeStr());
						deletedStResult = false;
						break;
					} else {
						logger.debug("Deleted Statement {} is an init statement",insertedSt.getEscapedCodeStr());
					}
				}
				jsonWriter.write("\"deletedStatements\":" + deletedStatements.size() + ",");

			} else {
				jsonWriter.write("\"deletedStatements\":\"None\"" + ",");
				deletedStResult = true;
			}
			jsonWriter.write("\"DeletedStatementsResult\":" + deletedStResult + ",");

			if(!deletedStResult) {
				retVal = false;
			}
		}

		if(retVal) {
			// check moved statements
			boolean movedStResult = false;
			List<CallExpression> updatedFuncStmts = new ArrayList<CallExpression>();
			MovedStatementChecker mvdStChecker = new MovedStatementChecker(targetMap);
			List<StatementMap> movedStatements = getTargetStatements(targetMap, currChecker, MAP_TYPE.MOVE);
			movedStResult = mvdStChecker.areMovedStatementsSafe(jsonWriter, movedStatements);
			jsonWriter.write("\"MovedStatementsResult\":" + movedStResult + ",");
			// get all moved call statements
			if(movedStResult) {
				// if the moved statements are fine.
				// get all call expressions that have been moved.
				for(StatementMap sm: movedStatements) {
					if(sm.getNew() instanceof Statement) {
						Statement newSt = (Statement)sm.getNew();
						if(newSt.getChild(0) instanceof CallExpression) {
							CallExpression insr = (CallExpression)(newSt.getChild(0));
							if(!(insr.getChild(1).getChildCount() > 0)) {
								// the updated call statements should not have any arguments
								updatedFuncStmts.add((CallExpression)newSt.getChild(0));
							}
						}
					}
				}
				// multiple functions have been re-ordered.
				if(!updatedFuncStmts.isEmpty()) {
					jsonWriter.write("\"HasReorderedFunctions\":" + true + ",");
				}
			}
			retVal = movedStResult;
		}
		
		
		jsonWriter.write("\"safepatch\":" + Boolean.toString(retVal));
		jsonWriter.write(",\"OldFuncErrorNodes\":" + Integer.toString(oldFuncErrorNodes.size()));
		jsonWriter.write(",\"NewFuncErrorNodes\":" + Integer.toString(newFuncErrorNodes.size()));
		return retVal;
	}
	
	
	/***
	 *  Get all the statements (inserted/updated/deleted statements) 
	 *  that do not affect any conditions.
	 *  
	 * @param targetMap target FunctionMap.
	 * @param currChecker target flow restrictive checker to use.
	 * @param targetType Target type of the statements to be fetched.
	 * @return List of target statement.
	 */
	private static List<StatementMap> getTargetStatements(FunctionMap targetMap, FlowRestrictiveChecker currChecker, MAP_TYPE targetType) {
		ArrayList<StatementMap> toRet = new ArrayList<StatementMap>();
		boolean needToAdd = false;
		for(StatementMap currMap: targetMap.getStatementMap()) {
			if(currMap.getMapType() == targetType) {
				ASTNode originalNode = currMap.getOriginal();
				ASTNode newNode = currMap.getNew();
				needToAdd = false;
				
				if(!(originalNode instanceof Condition) && 
						!(newNode instanceof Condition)) {
					if(!currChecker.isoldASTNodeAffectingStatement(originalNode) && 
					   !currChecker.isnewASTNodeAffectingStatement(newNode)) {
						needToAdd = true;
					}
				}
				
				// if the old node or new node in error bb.
				// ignore this statement map.
				if(currChecker.isOldNodeInErrorBB(originalNode) || currChecker.isNewNodeInErrorBB(newNode)) {
					if(needToAdd) {
						if(currChecker.isOldNodeInErrorBB(originalNode)) {
							oldFuncErrorNodes.add(originalNode);
						} 
						if(currChecker.isNewNodeInErrorBB(newNode)) {
							newFuncErrorNodes.add(newNode);
						}
					}
					continue;
				}
				
				if(needToAdd) {
					toRet.add(currMap);
				}
				
			}
		}
		
		return toRet;
		
	}
	
	
	/***
	 *  Check if the difference introduced by the patch is analyzable or not.
	 * 
	 * @param oldFunctions List of functions in the old file.
	 * @param newFunctions List of functions in the new file.
	 * @return NULL if analyzable else string representing the reason.
	 */
	public static String isDiffAnalyzable(ArrayList<FunctionDef> oldFunctions, ArrayList<FunctionDef> newFunctions) {
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
	 * Pre-process the provided file into the destination file.
	 * 
	 * @param srcFile source file.
	 * @param dstFile destination file where the pre-processed output
	 *                should be stored.
	 * @return true if everything is successful.
	 */
	public static boolean preprocessFile(File srcFile, File dstFile) {
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
