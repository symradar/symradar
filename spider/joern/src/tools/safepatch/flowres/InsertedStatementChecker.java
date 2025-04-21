/**
 * 
 */
package tools.safepatch.flowres;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.statements.Condition;
import ast.statements.Statement;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.heuristics.CodeHeuristics;

/**
 * @author machiry
 *
 */
public class InsertedStatementChecker {
	private FunctionMap targetMap = null;
	private static final Logger logger = LogManager.getLogger();
	private CodeHeuristics targetHeuristics = new CodeHeuristics();
	private FlowRestrictiveChecker targetChecker = null;
	
	public InsertedStatementChecker(FunctionMap fmap, FlowRestrictiveChecker targetCh) {
		this.targetMap = fmap;
		this.targetChecker = targetCh;
		
	}
	
	public boolean areAnyStatementsInserted(PrintWriter jsonWriter) {
		boolean retVal = true;
		List<StatementMap> allStMap = this.targetMap.getStatementMap();
		logger.debug("Computing the inserted statements to check");
		List<ASTNode> targetStmtsToCheck = new ArrayList<ASTNode>();
		
		for(StatementMap currStMap:allStMap) {
			ASTNode targetNewSt = currStMap.getNew();
			if(currStMap.getMapType() == MAP_TYPE.INSERT && targetNewSt != null) {
				assert(targetNewSt != null && currStMap.getOriginal() == null);
				BasicBlock targetNewBB =  this.targetMap.getNewClassicCFG().getBB(targetNewSt);
				// only if the insertion is not in error handling BB
				if(!targetNewBB.isConfigErrorHandling()) {
					if(!(targetNewSt instanceof Condition)) {
						targetStmtsToCheck.add(targetNewSt);
					}
				} else {
					logger.debug("Ignoring statement {} as it is in error handling BB", targetNewSt.getEscapedCodeStr());
				}
			}
		}
		logger.debug("Computed statements to check");
		jsonWriter.write("\"numInserted\":" + Integer.toString(targetStmtsToCheck.size()) + ",");
		
		return targetStmtsToCheck.size() != 0;
	}
	
	public boolean areInsertedStatementsSafe(PrintWriter jsonWriter) {
		boolean retVal = true;
		List<StatementMap> allStMap = this.targetMap.getStatementMap();
		logger.debug("Computing the inserted statements to check");
		List<ASTNode> targetStmtsToCheck = new ArrayList<ASTNode>();
		
		for(StatementMap currStMap:allStMap) {
			ASTNode targetNewSt = currStMap.getNew();
			if(currStMap.getMapType() == MAP_TYPE.INSERT && targetNewSt != null) {
				assert(targetNewSt != null && currStMap.getOriginal() == null);
				BasicBlock targetNewBB =  this.targetMap.getNewClassicCFG().getBB(targetNewSt);
				// only if the insertion is not in error handling BB
				if(!targetNewBB.isConfigErrorHandling()) {
					if(!(targetNewSt instanceof Condition)) {
						targetStmtsToCheck.add(targetNewSt);
					}
				} else {
					logger.debug("Ignoring statement {} as it is in error handling BB", targetNewSt.getEscapedCodeStr());
				}
			}
		}
		logger.debug("Computed statements to check");
		
		jsonWriter.write("\"numInserted\":" + Integer.toString(targetStmtsToCheck.size()) + ",");
		
		
		if(targetStmtsToCheck.size() > 0) {
			// for each of the inserted statements check, if they fall into one of out heuristics.
			for(ASTNode currStNode:targetStmtsToCheck) {
				if(currStNode instanceof Statement && 
				   !targetHeuristics.isInitNew((Statement)currStNode) && 
				   this.targetChecker.isNewStmtAffectsCondtion(currStNode)) {
					logger.debug("Inserted statement {} is not an init statement", currStNode.getEscapedCodeStr());
					retVal = false;
				} else {
					logger.debug("Inserted Statement {} is an init statement",currStNode.getEscapedCodeStr());
				}
			}
		} else {
			logger.debug("No valid inserted statements detected.");
		}
		
		jsonWriter.write("\"insertedStCheck\":" + Boolean.toString(retVal) + ",");
		return retVal;
	}
}
