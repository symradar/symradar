package tools.safepatch.diff;

import ast.functionDef.FunctionDef;
import cdg.CDG;
import cdg.CDGCreator;
import cdg.DominatorTree;
import cdg.ReverseCFG;
import cfg.ASTToCFGConverter;
import cfg.CFG;
import cfg.nodes.CFGNode;
import ddg.CFGAndUDGToDefUseCFG;
import ddg.DDGCreator;
import ddg.DataDependenceGraph.DDG;
import ddg.DataDependenceGraph.DDGDifference;
import ddg.DefUseCFG.DefUseCFG;
import udg.CFGToUDGConverter;
import udg.useDefGraph.UseDefGraph;

/**
 * @author Eric Camellini
 *
 */
public class JoernGraphs {
	
	private FunctionDef function;
	
	private CFG cfg;
	
	private CDG cdg;
	
	private UseDefGraph udg;
	
	private DefUseCFG defUseCfg;
	
	private DDG ddg;
	
	private DominatorTree<CFGNode> DomTree = null;
	private DominatorTree<CFGNode> PostDomTree = null;
	
	public JoernGraphs(FunctionDef f) {
		//The ASTDelta contains original and new AST
		this.function = f;
		
		
		//Creating the two CFGs
		ASTToCFGConverter astToCfg = new ASTToCFGConverter();
		this.cfg = astToCfg.convert(f);
		
		/*
		 * TODO
		 * Try to use directly the CFG Factory to extract CFG from Compound
		 * CFGFactory factory = new CCFGFactory(); etc...
		 */
		
		CDGCreator cdgCreator = new CDGCreator();
		this.cdg = cdgCreator.create(this.cfg);
		
		CFGToUDGConverter udgConv = new CFGToUDGConverter();
		this.udg = udgConv.convert(this.cfg);
		
		CFGAndUDGToDefUseCFG defUseCfgCreator = new CFGAndUDGToDefUseCFG();
		this.defUseCfg = defUseCfgCreator.convert(this.cfg, this.udg);
		
		DDGCreator ddgCreator = new DDGCreator();
		this.ddg = ddgCreator.createForDefUseCFG(this.defUseCfg);
		
	}

	
	public FunctionDef getFunction() {
		return function;
	}


	public CFG getCfg() {
		return cfg;
	}

	public CDG getCdg() {
		return cdg;
	}

	public UseDefGraph getUdg() {
		return udg;
	}

	public DefUseCFG getDefUseCfg() {
		return defUseCfg;
	}

	public DDG getDdg() {
		return ddg;
	}
	
	public DominatorTree<CFGNode> getDomTree() {
		if(this.DomTree == null) {
			this.DomTree = DominatorTree.newInstance(cfg, cfg.getEntryNode());
		}
		return this.DomTree;
	}
	
	public DominatorTree<CFGNode> getPostDomTree() {
		if(this.PostDomTree == null) {
			ReverseCFG reverseCFG = ReverseCFG.newInstance(cfg);
			this.PostDomTree = DominatorTree.newInstance(reverseCFG, reverseCFG.getEntryNode());
		}
		return this.PostDomTree;
	}
	
}
