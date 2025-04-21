package tools.safepatch.z3stuff;

import java.util.HashMap;
import java.util.HashSet;

import com.microsoft.z3.Context;

import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.flowres.FunctionMap;
import tools.safepatch.z3stuff.exprhandlers.IdentifierExprHandler;
import ast.ASTNode;

/***
 * 
 * @author machiry
 *
 */
public class GlobalZ3ObjManager {
	/* Map between ASTNode and the Z3 objects of all the used variables in the ASTNode
	 Example:
	 	{
	    (a =  b + 2) -> {"b" -> z3 object, "2" -> z3 object},
	    (c = a + w) -> {"a" -> z3 object, "w" -> z3 object}
	    }
	 */
	
	private static HashMap<ASTNode, HashMap<String, Object>> oldAstNodeCache = new HashMap<ASTNode, HashMap<String, Object>>();
	// Same as the above, but for new function.
	private static HashMap<ASTNode, HashMap<String, Object>> newAstNodeCache = new HashMap<ASTNode, HashMap<String, Object>>();
	
	private static HashMap<HashSet<ASTNode>, HashMap<String, Object>> oldmultiDefsASTNodeCache = new HashMap<HashSet<ASTNode>, HashMap<String, Object>>();
	private static HashMap<HashSet<ASTNode>, HashMap<String, Object>> newmultiDefsASTNodeCache = new HashMap<HashSet<ASTNode>, HashMap<String, Object>>();
	
	private static FunctionMap targetMap = null;
	
	private static HashMap<String, Object> identifierObjects = new HashMap<String, Object>();
	
	/***
	 * 
	 * @param currMap
	 * @return
	 */
	public static FunctionMap setFunctionMap(FunctionMap currMap) {
		FunctionMap oldMap = GlobalZ3ObjManager.targetMap;
		// clean up
		oldAstNodeCache.clear();
		newAstNodeCache.clear();
		GlobalZ3ObjManager.targetMap = currMap;
		GlobalZ3ObjManager.identifierObjects.clear();
		return oldMap;
	}
	
	public static void putIdentifierSymbol(String symName, Object targetObj) {
		GlobalZ3ObjManager.identifierObjects.put(symName, targetObj);
	}
	
	public static Object getIdentifierSymbol(Context ctx, String symName) {
		Object retVal = IdentifierExprHandler.handleKnownPredefinedConstants(ctx, symName); 
		if(GlobalZ3ObjManager.identifierObjects.containsKey(symName)) {
			retVal = GlobalZ3ObjManager.identifierObjects.get(symName);
		}
		return retVal;
	}
	
	/***
	 * 
	 * @param currNode
	 * @param symbolName
	 * @return
	 */
	public static Object getZ3ObjSymbol(ASTNode currNode, String symbolName, boolean fromOld) {
		assert(GlobalZ3ObjManager.targetMap != null);
		Object toRet = null;
		HashMap<ASTNode, HashMap<String, Object>> targetNodeCache = null;
		// if the symbol is from the new function.
		if(fromOld) {
			// old
			targetNodeCache = GlobalZ3ObjManager.oldAstNodeCache;
		} else {
			// new
			targetNodeCache = GlobalZ3ObjManager.newAstNodeCache;
		}
		// Case when the symbol is already present.
		if(targetNodeCache.containsKey(currNode) && targetNodeCache.get(currNode).containsKey(symbolName)) {
			toRet = targetNodeCache.get(currNode).get(symbolName);
		}		
		return toRet;
	}
	
	
	/***
	 * 
	 * @param currNode
	 * @param targetDefNodes
	 * @param symbolName
	 * @return
	 */
	public static Object getMultidefZ3ObjSymbol(ASTNode currNode, HashSet<ASTNode> targetDefNodes, String symbolName, boolean fromOld) {
		assert(GlobalZ3ObjManager.targetMap != null);
		Object toRet = null;
		HashMap<HashSet<ASTNode>, HashMap<String, Object>> targetNodeCache = null;
		// if the symbol is from the new function.
		if(fromOld) {
			// old
			targetNodeCache = GlobalZ3ObjManager.oldmultiDefsASTNodeCache;
		} else {
			// new
			targetNodeCache = GlobalZ3ObjManager.newmultiDefsASTNodeCache;
		}
		// Case when the symbol is already present.
		if(targetNodeCache.containsKey(targetDefNodes) && 
		   targetNodeCache.get(targetDefNodes).containsKey(symbolName)) {
			
			toRet = targetNodeCache.get(targetDefNodes).get(symbolName);
		}		
		return toRet;
	}
	
	
	/***
	 * 
	 * @param currNode
	 * @param symbolName
	 * @param targetObj
	 * @param fromOld
	 * @param createNew
	 */
	public static void putZ3ObjSymbol(ASTNode currNode, String symbolName, Object targetObj, boolean fromOld, boolean createNew) {
		assert(GlobalZ3ObjManager.targetMap != null);
		HashMap<ASTNode, HashMap<String, Object>> targetNodeCache = null;
		// put into old cache.
		if(fromOld) {
			targetNodeCache = GlobalZ3ObjManager.oldAstNodeCache;
		} else {
			targetNodeCache = GlobalZ3ObjManager.newAstNodeCache;
		}
		
		if(!targetNodeCache.containsKey(currNode)) {
			targetNodeCache.put(currNode, new HashMap<String, Object>());
		}
		HashMap<String, Object> targetCacheObj = targetNodeCache.get(currNode);
		targetCacheObj.put(symbolName, targetObj);
		
		if(fromOld && !createNew) {
			StatementMap targetM = GlobalZ3ObjManager.targetMap.getTargetStMap(currNode, fromOld);
			if(targetM != null && targetM.getNew() != null) {
				ASTNode newFnNode = targetM.getNew();
				targetNodeCache = GlobalZ3ObjManager.newAstNodeCache;
				if(!targetNodeCache.containsKey(newFnNode)) {
					targetNodeCache.put(newFnNode, new HashMap<String, Object>());
				}
				targetCacheObj = targetNodeCache.get(newFnNode);
				targetCacheObj.put(symbolName, targetObj);
			}
			
		}
	}
	
	
	/***
	 * 
	 * @param currNode
	 * @param targetDefNodes
	 * @param symbolName
	 * @param targetObj
	 * @param fromOld
	 * @param createNew
	 */
	public static void putMultidefZ3ObjSymbol(ASTNode currNode, HashSet<ASTNode> targetDefNodes, String symbolName, Object targetObj, boolean fromOld, boolean createNew) {
		assert(GlobalZ3ObjManager.targetMap != null);
		HashMap<HashSet<ASTNode>, HashMap<String, Object>> targetNodeCache = null;
		// put into old cache.
		if(fromOld) {
			targetNodeCache = GlobalZ3ObjManager.oldmultiDefsASTNodeCache;
		} else {
			targetNodeCache = GlobalZ3ObjManager.newmultiDefsASTNodeCache;
		}
		
		if(!targetNodeCache.containsKey(targetDefNodes)) {
			targetNodeCache.put(targetDefNodes, new HashMap<String, Object>());
		}
		
		HashMap<String, Object> targetCacheObj = targetNodeCache.get(targetDefNodes);
				
		targetCacheObj.put(symbolName, targetObj);
		
		if(fromOld && !createNew) {
			
			HashSet<ASTNode> newASTNodes = new HashSet<ASTNode>();
			for(ASTNode oldChASTNode: targetDefNodes) {
				StatementMap targetM = GlobalZ3ObjManager.targetMap.getTargetStMap(oldChASTNode, fromOld);
				if(targetM != null && targetM.getNew() != null) {
					newASTNodes.add(targetM.getNew());
				}
			}
				
			if(!newASTNodes.isEmpty()) {
				targetNodeCache = GlobalZ3ObjManager.newmultiDefsASTNodeCache;
				if(!targetNodeCache.containsKey(newASTNodes)) {
					targetNodeCache.put(newASTNodes, new HashMap<String, Object>());
				}
				
				targetCacheObj = targetNodeCache.get(newASTNodes);
					
				targetCacheObj.put(symbolName, targetObj);
			}
			
		}
	}
}
