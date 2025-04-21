/**
 * 
 */
package tools.safepatch.bberr;

import java.io.IOException;
import java.util.ArrayList;

import tools.safepatch.cfgclassic.BasicBlockUtils;
import tools.safepatch.cfgclassic.ClassicCFG;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.heuristics.CodeHeuristics;
import ast.functionDef.FunctionDef;
import tools.safepatch.cfgclassic.BasicBlock;

/**
 * @author machiry
 *
 */
public class ErrorBBDetector {
	private FunctionDef providedFuncDef = null;
	private JoernGraphs targetGraphs = null;
	private ClassicCFG bCFG = null; 
	public static float ERRORTHRESHOLD = (float) 2.0;
	private String funcName = null;
	
	private static boolean USE_STATEMENT_COVERAGE_RATIO = false;
	
	private static CodeHeuristics codeHeu = new CodeHeuristics();
	
	public ErrorBBDetector(FunctionDef targetFunction) {
		this.providedFuncDef = targetFunction;
		this.funcName = this.providedFuncDef.getFunctionSignature().split("\\(")[0].trim();
		// generate all the joern graphs.
		this.targetGraphs = new JoernGraphs(this.providedFuncDef);
		this.bCFG = new ClassicCFG(this.targetGraphs.getCfg(), this.targetGraphs.getCdg(), this.providedFuncDef);
	}
	
	public String identifyAllErrorBBs() throws IOException {
		return ErrorBBDetector.identifyAllErrorBBs(this.bCFG);
	}
	
	/***
	 *  The main core function that identifies all the error
	 *  basic blocks within the provided function.
	 * @throws IOException 
	 */
	public static String identifyAllErrorBBs(ClassicCFG targetCFG) throws IOException {
		
		
		
		ArrayList<BasicBlock> potentialErrorBBs = new ArrayList<BasicBlock>();
		String retVal = "{\"function\":\"" + targetCFG.funcName + "\", \"berr\":[";
		
		boolean needComma = false;
		boolean isHandled = false;
		for(BasicBlock currBB:targetCFG.getAllBBs()) {
		
			Object[] sucessorBBs = currBB.bbEdges.keySet().toArray();
			// only if it has more than one child.
			if(sucessorBBs.length > 1) {
				// as of now we only handle if Basic blocks.
				// we may need to handle switch and shit.
				if(sucessorBBs.length == 2) {
					isHandled = false;
					BasicBlock firstChild = (BasicBlock) sucessorBBs[0];
					BasicBlock secondChild = (BasicBlock) sucessorBBs[1];
					
					if(codeHeu.isError(firstChild.getAllASTNodes())) {
						if(needComma) {
							retVal += ",";
						}
						
						retVal += currBB.getStartLine();
						currBB.setErrorGuarding(true);
						firstChild.setErrorHandling(true);
						currBB.addErrorHandlingChild(firstChild);
						needComma = true;
						isHandled = true;
					} else {
						if(codeHeu.isError(secondChild.getAllASTNodes())) {
							if(needComma) {
								retVal += ",";
							}
							
							retVal += currBB.getStartLine();
							currBB.setErrorGuarding(true);
							secondChild.setErrorHandling(true);
							currBB.addErrorHandlingChild(secondChild);
							needComma = true;
							isHandled = true;
						}
					}
					
					// switch to enable and disable the ration trick.
					if(ErrorBBDetector.USE_STATEMENT_COVERAGE_RATIO) {
						if(BasicBlockUtils.isInLoop(currBB) || isHandled) {
							continue;
						}
						
						
					
						float firstNumSt = (float)firstChild.getNumReachableStatments(null);
						float secondNumSt = (float)secondChild.getNumReachableStatments(null);
						float fs = firstNumSt/secondNumSt;
						float sf = secondNumSt/firstNumSt;
						if(fs > ErrorBBDetector.ERRORTHRESHOLD || sf > ErrorBBDetector.ERRORTHRESHOLD) {
							if(needComma) {
								retVal += ",";
							}
							
							retVal += currBB.getStartLine();
							currBB.setErrorGuarding(true);
							if(fs > ErrorBBDetector.ERRORTHRESHOLD) {
								secondChild.setErrorHandling(true);
								currBB.addErrorHandlingChild(secondChild);
							} else {
								firstChild.setErrorHandling(true);
								currBB.addErrorHandlingChild(firstChild);
							}
							
							/*System.out.println("For Function:" + funcName + ":" + this.providedFuncDef.getLocation().startLine + ":" + this.providedFuncDef.getLocation().stopLine);
							System.out.println("BBSTART:" + currBB.getStartLine());
							System.out.println(currBB.toString());
							System.out.println("BBEND");*/
							needComma = true;
						}
					}
				}
			}
		}
		
		retVal += "]\n}";
		// propagate error basic block info
		/*for(BasicBlock currBB: targetCFG.getAllBBs()) {
			currBB.propagateErrorHandling();
		}*/
		return retVal;
		
		// OK, first we need to find all
		
		/*CFG targetFunctionCFG = this.targetGraphs.getCfg();
		for(CFGNode currCFG: targetFunctionCFG.getVertices()) {
			System.out.println(currCFG.getClass().toString());
		}
		
		String targetFuncName = this.providedFuncDef.getFunctionSignature().split("\\(")[0].trim();
		
		ClassicCFG bCFG = new ClassicCFG(targetFunctionCFG, this.providedFuncDef);
		
		String dotOutput = bCFG.getDigraphOutput();
		File dotOutputFile = new File("/home/machiry/Desktop/funcPngDir/", targetFuncName + ".dot");
		if(!dotOutputFile.exists()) {
			dotOutputFile.createNewFile();
		}
		
		PrintWriter prWri = new PrintWriter(dotOutputFile);
		prWri.print(dotOutput);
		prWri.close();
		
		
		for(CFGEdge cfgEdg: targetFunctionCFG.getEdges()) {
			System.out.println(cfgEdg.getSource() + "\n");
			System.out.println(cfgEdg.getProperties().toString() + "\n");
			System.out.println(cfgEdg.getDestination() + "\n");
		}*/
		
		
	}
}
