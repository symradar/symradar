package tools.safepatch;

import java.util.ArrayList;
import java.util.List;

import ast.functionDef.FunctionDef;
import sun.reflect.generics.reflectiveObjects.NotImplementedException;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.GumtreeASTMapper;


/**
 * @author Eric Camellini
 *
 */
public class FunctionMatcher {

	public enum MATCHING_METHOD{
		NAME_ONLY,
		AST_MATCHING
	}
	
	private List<FunctionDef> oldFunctionList;
	private List<FunctionDef> newFunctionList;
	private List<FunctionDiff> commonFunctions = null;
	private GumtreeASTMapper mapper;
	
	private MATCHING_METHOD method;
	
	public FunctionMatcher(List<FunctionDef> oldFunctionList, List<FunctionDef> newFunctionList, MATCHING_METHOD method) {
		super();
		this.oldFunctionList = oldFunctionList;
		this.newFunctionList = newFunctionList;
		this.method = method;
	}
	
	/**
	 * Performs the matching. Matched functions can then be get using the getCommonFunctions() method.
	 */
	public void match(){
		this.commonFunctions = new ArrayList<FunctionDiff>();
		switch (this.method) {
		case AST_MATCHING:
			if(this.mapper == null)
				throw new RuntimeException("Cannot perform AST method matching: must first set a GumtreeASTMapper");
			this.astMatching();
			break;

		case NAME_ONLY:
			this.nameMatching();
			break;
			
		default:
			break;
		}
	}
	
	/**
	 * Returns the function pairs matched when the match() method was called.
	 */
	public List<FunctionDiff> getCommonFunctions(){
		if(this.commonFunctions == null)
			throw new RuntimeException("Trying to get matched functions, but match() method was never called.");
		return this.commonFunctions;
	}
	
	public void setGumtreeASTMapper(GumtreeASTMapper mapper){
		this.mapper = mapper;
	}
	
	private void astMatching(){
		//TODO Function matching using AST matching like described in the paper by 
		//Kreutzer et al. "Automatic Clustering of Code Changes"
		throw new NotImplementedException();
	}

	private void nameMatching(){
		for(FunctionDef oldFunc: oldFunctionList){
			for(FunctionDef newFunc : newFunctionList){
				if(oldFunc.name.getEscapedCodeStr().equals(newFunc.name.getEscapedCodeStr()))
					this.commonFunctions.add(new FunctionDiff(null, oldFunc, null, newFunc));
			}
		}
	}
	
}
