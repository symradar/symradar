package tools.safepatch.heuristics;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.statements.Condition;
import tools.safepatch.conditions.ConditionDiff;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;

public class HeuristicsHelper {

	private static final Logger logger = LogManager.getLogger();
	
	/*
	 * Returns all the conditions that are affected by the change and present
	 * in both files. This method does not return conditions that are:
	 * 1) Completely new (only inserted nodes) 
	 * 2) Completely removed (only deleted nodes)
	 * 3) Completely moved but unchanged (only moved nodes)
	 */
	public static List<ConditionDiff> getModifiedConditions(ASTDiff astDiff, Map<String, String> typesMap, boolean considerMovedConditions, boolean bvEnabled){
		//1) We extract all the conditions that are affected by some kind of action
		List<ASTNode> affectedConditons = astDiff
				.getNewAffectedNodes(Condition.class);
		List<ConditionDiff> cDiffs = new ArrayList<ConditionDiff>();
		//2) For every affected condition...
		for(ASTNode c : affectedConditons){
			Condition newCond = (Condition) c;
			//We extract the corresponding old one
			Condition oldC = (Condition) astDiff.getMappedNode(newCond);

			//If it exists
			if(oldC != null){
				//We consider condition purely moved according to the parameter
				if((!considerMovedConditions && !astDiff.areAllNodesMarkedAs(newCond, ACTION_TYPE.MOVE)) || 
						considerMovedConditions){
					//We create the condition diff object and we add it to the list to be returned
					ConditionDiff cDiff = new ConditionDiff(oldC, newCond, astDiff);
					cDiff.setTypesMap(typesMap);
					cDiff.setBVEnabled(bvEnabled);
					cDiffs.add(cDiff);
				}
			} 	
		}
		
		return cDiffs;
	}
	
	/**
	 * Returns the ConditionDiffs for witch the following holds:
	 * (original -> new) AND NOT (new <-> original) 
	 */
	public static List<ConditionDiff> getOriginalImpliesNew(List<ConditionDiff> cDiffs) {
		return cDiffs.stream().filter(cDiff -> 
		(!cDiff.getBooleanAndArithmeticDiff().biImplication() &&
				cDiff.getBooleanAndArithmeticDiff().originalImpliesNew())).collect(Collectors.toList());
	}
	
	/**
	 * Returns the ConditionDiffs for witch the following holds:
	 * (new -> original) AND NOT (new <-> original) 
	 */
	public static List<ConditionDiff> getNewImpliesOriginal(List<ConditionDiff> cDiffs) {
		return cDiffs.stream().filter(cDiff -> 
		(!cDiff.getBooleanAndArithmeticDiff().biImplication() &&
				cDiff.getBooleanAndArithmeticDiff().newImpliesOriginal())).collect(Collectors.toList());
	}
	
	/**
	 * Returns the ConditionDiffs for witch the following holds:
	 * (new <-> original) 
	 */
	public static List<ConditionDiff> getBiImplications(List<ConditionDiff> cDiffs) {
		return cDiffs.stream().filter(cDiff -> 
		cDiff.getBooleanAndArithmeticDiff().biImplication()).collect(Collectors.toList());
	}
	
	public static boolean anyConditionImplication(ASTDiff diff, Map<String, String> typesMap, boolean considerMovedConditions, boolean bvEnabled){
		List<ConditionDiff> cDiffs = HeuristicsHelper.getModifiedConditions(diff, typesMap, considerMovedConditions, bvEnabled);
		if(logger.isInfoEnabled()) logger.info("Modified conditions: \n{}", cDiffs);
		return getNewImpliesOriginal(cDiffs).size() > 0 || getOriginalImpliesNew(cDiffs).size() > 0;
	}
	
}
