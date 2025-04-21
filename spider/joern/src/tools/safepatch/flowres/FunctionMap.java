/**
 * 
 */
package tools.safepatch.flowres;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.microsoft.z3.Context;

import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import tools.safepatch.bberr.ErrorBBDetector;
import tools.safepatch.cfgclassic.ClassicCFG;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.fgdiff.GumtreeASTMapper;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import tools.safepatch.cfgclassic.BasicBlockMap;

/**
 * @author machiry
 *
 */
public class FunctionMap {
	private FunctionDef oldFunction = null;
	private FunctionDef newFunction = null;
	private JoernGraphs originalGraphs = null;
	private JoernGraphs newGraphs = null;
	private ClassicCFG boldCFG = null; 
	private ClassicCFG bnewCFG = null; 
	
	private List<BasicBlockMap> bbMap = null;
	private boolean areFunctionsSame = true;
	private HashMap<ASTNode, ASTNode> mappedConditions = null;
	private List<StatementMap> allStatementMap = null;
	private List<ASTNode> insertedConditions = null;
	private ASTNodeToZ3Converter oldZ3Convertor = null;
	private ASTNodeToZ3Converter newZ3Convertor = null;
	
	private Context targetCtx = null;
	
	public FunctionMap(FunctionDef oldf, FunctionDef newf) {
		this.oldFunction = oldf;
		this.newFunction = newf;
	}
	
	
	public FunctionDef getOldFunction() {
		return this.oldFunction;
	}
	
	public FunctionDef getNewFunction() {
		return this.newFunction;
	}
	
	public boolean areSame() {
		return this.areFunctionsSame;
	}
	
	public List<StatementMap> getStatementMap() {
		return this.allStatementMap;
	}
	
	public JoernGraphs getOldFuncGraphs() {
		return this.originalGraphs;
	}
	
	public JoernGraphs getNewFuncGraphs() {
		return this.newGraphs;
	}
	
	public HashMap<ASTNode, ASTNode> getMappedConditions() {
		return this.mappedConditions;
	}
	
	public List<ASTNode> getInsertedConditions() {
		return this.insertedConditions;
	}
	
	public ClassicCFG getOldClassicCFG() {
		return this.boldCFG;
	}
	
	public ClassicCFG getNewClassicCFG() {
		return this.bnewCFG;
	}
	
	public ASTNodeToZ3Converter getOldFnConv() {
		return this.oldZ3Convertor;
	}
	public ASTNodeToZ3Converter getNewFnConv() {
		return this.newZ3Convertor;
	}
	
	public void setOldFnConv(ASTNodeToZ3Converter oldC) {
		this.oldZ3Convertor = oldC;
	}
	
	public void setNewFnConv(ASTNodeToZ3Converter newC) {
		this.newZ3Convertor = newC;
	}
	
	public Context getZ3Ctx() {
		return this.targetCtx;
	}
	
	public void setZ3Ctx(Context ctx) {
		this.targetCtx = ctx;
	}
	
	public StatementMap getTargetStMap(ASTNode currNode, boolean fromOld) {
		StatementMap toRet = null;
		for(StatementMap currM:this.allStatementMap) {
			if(fromOld) {
				if(currM.getOriginal() != null && currM.getOriginal().equals(currNode)) {
					toRet = currM;
					break;
				}
			} else {
				if(currM.getNew() != null && currM.getNew().equals(currNode)) {
					toRet = currM;
					break;
				}
			}
		}
		return toRet;
	}
	
	/***
	 * 
	 * @return
	 */
	public boolean processFunctions() {
		boolean retVal = true;
		//TODO: finish this.
		// 1. generate Joern graphs.
		try {
			if(this.originalGraphs == null) {
				this.originalGraphs = new JoernGraphs(this.oldFunction);
			}
			if(this.newGraphs == null) {
				this.newGraphs = new JoernGraphs(this.newFunction);
			}
			// 2. generate Classic CFGs
			if(this.boldCFG == null) {
				this.boldCFG = new ClassicCFG(this.originalGraphs.getCfg(), this.originalGraphs.getCdg(), this.oldFunction);
				// find all error bbs in old CFG
				ErrorBBDetector.identifyAllErrorBBs(this.boldCFG);
			}
			if(this.bnewCFG == null) {
				this.bnewCFG = new ClassicCFG(this.newGraphs.getCfg(), this.newGraphs.getCdg(), this.newFunction);
				// find all error bbs in new CFG
				ErrorBBDetector.identifyAllErrorBBs(this.bnewCFG);
			}
			if(this.bbMap == null) {		
				this.bbMap = new ArrayList<BasicBlockMap>();
				List<CFGNode> oldCFGNodes = this.originalGraphs.getCfg().getVertices();
				List<CFGNode> newCFGNodes = this.newGraphs.getCfg().getVertices();
				
				List<ASTNode> oldS = new ArrayList<ASTNode>();
				List<ASTNode> newS = new ArrayList<ASTNode>();
				
				for(CFGNode oc:oldCFGNodes) {
					if(oc instanceof ASTNodeContainer) {
						ASTNodeContainer currC = (ASTNodeContainer)oc;
						oldS.add(currC.getASTNode());						
					}
				}
				
				for(CFGNode oc:newCFGNodes) {
					if(oc instanceof ASTNodeContainer) {
						ASTNodeContainer currC = (ASTNodeContainer)oc;
						newS.add(currC.getASTNode());
					}
				}
				
				GumtreeASTMapper mappy = new GumtreeASTMapper();
				List<StatementMap> currStMap = mappy.getStatmentMap(this.oldFunction, this.newFunction, oldS, newS);
				this.allStatementMap = currStMap;
				for(StatementMap curM:currStMap) {
					if(curM.getMapType() != MAP_TYPE.UNMODIFIED && curM.getMapType() != MAP_TYPE.UNDEFINED) {
						this.areFunctionsSame = false;
					}
					BasicBlockMap.getorCreateMap(curM, this.bbMap, this.boldCFG.getAllBBs(), this.bnewCFG.getAllBBs());
				}
			}
		} catch(Exception e) {
			e.printStackTrace();
			retVal = false;
		}
		
		return retVal;
	}
	
	/***
	 * 
	 * @return
	 */
	public boolean computeMappedConditions() {
		this.processFunctions();
		try {
			if(this.mappedConditions == null) {
				this.mappedConditions = new HashMap<ASTNode, ASTNode>();
				for(BasicBlockMap currM:this.bbMap) {
					HashMap<ASTNode, ASTNode> mappedC = currM.getMappedConditions();
					/*if(mappedC.size() > 0) {
						for(ASTNode currK:mappedC.keySet()) {
							ASTNode currV = mappedC.get(currK);
							System.out.println("MAPPED COND:" + currK.getEscapedCodeStr() + "::::" + currV.getEscapedCodeStr());
						}
					}*/
					if(mappedC != null) {
						this.mappedConditions.putAll(mappedC);
					}
				}
			}
		} catch(Exception e) {
			e.printStackTrace();
			this.mappedConditions = null;
		}
		return this.mappedConditions != null;
	}
	
	
	public boolean computeInsertedConditions() {
		this.processFunctions();
		try {
			if(this.insertedConditions == null) {
				this.insertedConditions = new ArrayList<ASTNode>();
				for(StatementMap currStMap: this.allStatementMap) {
					if(currStMap.getMapType() == MAP_TYPE.INSERT && currStMap.getNew() instanceof Condition) {
						this.insertedConditions.add(currStMap.getNew());
					}
				}
				
			}
		} catch(Exception e) {
			e.printStackTrace();
			this.insertedConditions = null;
		}
		return this.insertedConditions != null;
	}
			

}
