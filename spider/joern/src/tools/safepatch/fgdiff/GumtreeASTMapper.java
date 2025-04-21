package tools.safepatch.fgdiff;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.omg.CORBA.portable.Delegate;

import ast.ASTNode;
import ast.CodeLocation;
import ast.functionDef.FunctionDef;
import ast.statements.CompoundStatement;
import ast.statements.Statement;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchMain;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.util.GumtreeUtil;
import tools.safepatch.util.JoernUtil;

import com.github.gumtreediff.actions.ActionGenerator;
import com.github.gumtreediff.actions.model.Action;
import com.github.gumtreediff.actions.model.Delete;
import com.github.gumtreediff.actions.model.Insert;
import com.github.gumtreediff.actions.model.Move;
import com.github.gumtreediff.actions.model.Update;
import com.github.gumtreediff.client.Run;
import com.github.gumtreediff.gen.Generators;
import com.github.gumtreediff.gen.jdt.JdtTreeGenerator;
import com.github.gumtreediff.matchers.Mapping;
import com.github.gumtreediff.matchers.MappingStore;
import com.github.gumtreediff.matchers.Matcher;
import com.github.gumtreediff.matchers.Matchers;
import com.github.gumtreediff.tree.ITree;
import com.github.gumtreediff.tree.Tree;
import com.github.gumtreediff.tree.TreeContext;

public class GumtreeASTMapper {
	
	private static final Logger logger = LogManager.getLogger();
	
	private static Map<Integer, String> typeLabelsMap = new HashMap<>();
	private Map<ASTNode, ITree> astNodeITreeMap = new HashMap<>();
	private Map<ITree, ASTNode> iTreeASTNodeMap = new HashMap<>();
	
	public static final String AST_NODE_ID_KEY = "astnodeid";
			
	/**
	 * double gumtree.match.xy.sim 0.5: similarity value for XyDiff.
	 */
	public void setXYSimilarity(double value){
		System.setProperty("gumtree.match.xy.sim", Double.toString(value));
	}
	
	/**
	 * int gumtree.match.gt.minh 2: minimum matching height for GumTree top-down phase.
	 */
	public void setTopDownMinHeight(int value){
		System.setProperty("gumtree.match.gt.minh", Integer.toString(value));
	}
	
	/**
	 * double gumtree.match.bu.sim 0.3: minimum similarity threshold for matching in GumTree bottom-up phase.
	 */
	public void setBottomUpSimilarity(double d){
		System.setProperty("gumtree.match.bu.sim", Double.toString(d));
	}
	
	/**
	 * double gumtree.match.bu.size 1000: maximum subtree size to apply the optimal algorithm in GumTree bottum-up phase.
	 */
	public void setBottomUpMaxSize(int value){
		System.setProperty("gumtree.match.bu.size", Integer.toString(value));
	}
	
	
	public static String getTypeLabel(int type){
		return typeLabelsMap.get(type);
	}
	
	private static void registerTypeLabel(int type, String name){
		String typeLabel = typeLabelsMap.get(type);
        if (typeLabel == null) {
            typeLabelsMap.put(type, name);
        } else if (!typeLabel.equals(name))
            throw new RuntimeException(String.format("Redefining type %d: '%s' with '%s'", type, typeLabel, name));
	}
	
	/*
	 * TODO:
	 * - Test better with different Gumtree parameters
	 * - Check if there is the need to add a different location information
	 * - Are there some other nodes that need to have a text label associated?
	 */
	
	
	public ASTDiff map(List<Statement> originalStatements, List<Statement> newStatements){
		String ROOT_TYPE = "EMPTY_ROOT";
		
		TreeContext context = new TreeContext(); 
		ITree oldTree = context.createTree(ROOT_TYPE.hashCode(),"",ROOT_TYPE);
		ITree newTree = context.createTree(ROOT_TYPE.hashCode(),"",ROOT_TYPE);
		for(Statement s : originalStatements){
			getITreeFromASTNode(s, context, oldTree);
		}
		
		for(Statement s : newStatements){
			getITreeFromASTNode(s, context, newTree);
		}
		
		Matcher m = Matchers.getInstance().getMatcher(oldTree, newTree); // retrieve the default matchers
		
		m.match();
		MappingStore mappings = m.getMappings(); // return the mapping store
		ActionGenerator g = new ActionGenerator(oldTree, newTree, mappings);
		g.generate();
		return new ASTDiff(mappings, g.getActions(), this);	
	}
	
	
	
	/***
	 * 
	 * @param currNode
	 * @param alStmts
	 * @return
	 */
	private static ASTNode getTargetStatement(ASTNode currNode, List<ASTNode> alStmts, boolean first) {
		ASTNode toRet = null;
		if(currNode != null) {
			if(alStmts.contains(currNode)) {
				return currNode;
			}
		}
		if(currNode != null) {
			toRet = getTargetStatement(currNode.getParent(), alStmts, false);
		}
		// OK this is last resort.
		// find the statement that has the same line number as the ASTNode and return it.
		
		if(toRet == null && currNode != null && first) {
			for(ASTNode currSt:alStmts) {
				if(currNode.getLocation().startLine == currSt.getLocation().startLine) {
					return currSt;
				}
			}
		}
		return toRet;
	}
	
	
	private boolean removeAllMaps(List<StatementMap> currStMap, ASTNode oldSt, ASTNode newSt) {
		boolean retVal = false;
		ArrayList<StatementMap> newStMap = new ArrayList<StatementMap>();
		newStMap.addAll(currStMap);
		
		for(StatementMap currM:currStMap) {
			if(currM.getMapType() != MAP_TYPE.UNMODIFIED && currM.getMapType() != MAP_TYPE.UPDATE
					&& currM.getMapType() != MAP_TYPE.MOVE) {
				if(currM.getNew() == newSt || currM.getOriginal() == oldSt) {
					newStMap.remove(currM);
				}
			}
		}
		retVal = newStMap.size() < currStMap.size();
		currStMap.clear();
		currStMap.addAll(newStMap);
		return retVal;
	}
	
	
	public List<StatementMap> getStatmentMap(FunctionDef originalF, FunctionDef newF, List<ASTNode> origStatements, List<ASTNode> newStatements) {
		ArrayList<StatementMap> retVal = new ArrayList<StatementMap>();
		
		ITree itree1 = getITreeFromASTNode(originalF);
		ITree itree2 = getITreeFromASTNode(newF);

		Matcher m = Matchers.getInstance().getMatcher(itree1, itree2); // retrieve the default matcher
		m.match();
		MappingStore mappings = m.getMappings(); // return the mapping store
		Set<Mapping> currMappings = mappings.asSet();
		/*for(Mapping currM:currMappings) {
			ITree currA = currM.first;
			ITree currB = currM.second;
			ASTNode aASTN = this.getMappedASTNode(currA);
			ASTNode bASTN = this.getMappedASTNode(currB);
			System.out.println("Mapped:" + aASTN.getEscapedCodeStr() + ", Second:" + bASTN.getEscapedCodeStr());
		}*/
		
		List<ASTNode> handledStatements = new ArrayList<ASTNode>(); 
		
		
		
		ActionGenerator g = new ActionGenerator(itree1, itree2, mappings);
		g.generate();
		
		List<Action> allActions = g.getActions();
		for(Action currAction:allActions) {
			ITree actionNode = currAction.getNode();
			ASTNode node = this.getMappedASTNode(actionNode);
			// this is an insert node.
			if(currAction instanceof Insert) {
				// only present in new
				// get the corresponding statement.
				ASTNode addedStmt = GumtreeASTMapper.getTargetStatement(node, newStatements, true);
				assert(addedStmt != null);
				if(addedStmt != null) {
					if(!handledStatements.contains(addedStmt)) {
						handledStatements.add(addedStmt);
						StatementMap newMap = new StatementMap(null, addedStmt, MAP_TYPE.INSERT);
						retVal.add(newMap);
					}
				}
			}
			if(currAction instanceof Delete) {
				// only present in old
				// get the corresponding statement.
				ASTNode deletedStmt = GumtreeASTMapper.getTargetStatement(node, origStatements, true);
				assert(deletedStmt != null);
				if(deletedStmt != null) {
					if(!handledStatements.contains(deletedStmt)) {
						handledStatements.add(deletedStmt);
						StatementMap newMap = new StatementMap(deletedStmt, null, MAP_TYPE.DELETE);
						retVal.add(newMap);
					}
				}
			}
			if(currAction instanceof Update) {
				// present in both old and new.
				ITree mTree = this.getMappedITree(node);
				ITree mappedDst = mappings.getDst(mTree);
				ASTNode dstNode = this.getMappedASTNode(mappedDst);
				
				ASTNode srcStatement = GumtreeASTMapper.getTargetStatement(node, origStatements, true);
				assert(srcStatement != null);
				ASTNode dstStatement = GumtreeASTMapper.getTargetStatement(dstNode, newStatements, true);
				assert(dstStatement != null);
				
				if(srcStatement != null && dstStatement != null && 
						!handledStatements.contains(srcStatement) && 
						!handledStatements.contains(dstStatement)) {
					StatementMap newMap = new StatementMap(srcStatement, dstStatement, MAP_TYPE.UPDATE);
					handledStatements.add(srcStatement);
					handledStatements.add(dstStatement);
					retVal.add(newMap);
				}
			}
			if(currAction instanceof Move) {
				ITree mTree = this.getMappedITree(node);
				ITree mappedDst = mappings.getDst(mTree);
				ASTNode dstNode = this.getMappedASTNode(mappedDst);
				
				ASTNode srcStatement = GumtreeASTMapper.getTargetStatement(node, origStatements, true);
				assert(srcStatement != null);
				ASTNode dstStatement = GumtreeASTMapper.getTargetStatement(dstNode, newStatements, true);
				assert(dstStatement != null);
				if(srcStatement != null && dstStatement != null) {
					if(!handledStatements.contains(srcStatement) && !handledStatements.contains(dstStatement)) {
						StatementMap newMap = new StatementMap(srcStatement, dstStatement, MAP_TYPE.MOVE);
						handledStatements.add(srcStatement);
						handledStatements.add(dstStatement);
						retVal.add(newMap);
					}
				}
			}
			
		}
		
		for(Mapping currMap:currMappings) {
			ITree fTree = currMap.getFirst();
			ITree dTree = currMap.getSecond();
			
			ASTNode srcNode = this.getMappedASTNode(fTree);			
			ASTNode dstNode = this.getMappedASTNode(dTree);
			
			ASTNode srcStatement = GumtreeASTMapper.getTargetStatement(srcNode, origStatements, true);
			assert(srcStatement != null);
			ASTNode dstStatement = GumtreeASTMapper.getTargetStatement(dstNode, newStatements, true);
			assert(dstStatement != null);
			
			if(srcStatement == null || dstStatement == null) {
				continue;
			}
			
			if(handledStatements.contains(srcStatement) || handledStatements.contains(dstStatement)) {
				if(removeAllMaps(retVal, srcStatement, dstStatement)) {
					StatementMap newMap = new StatementMap(srcStatement, dstStatement, MAP_TYPE.UPDATE);
					handledStatements.add(srcStatement);
					handledStatements.add(dstStatement);
					retVal.add(newMap);
				}
			} else {
				StatementMap newMap = new StatementMap(srcStatement, dstStatement, MAP_TYPE.UNMODIFIED);
				handledStatements.add(srcStatement);
				handledStatements.add(dstStatement);
				retVal.add(newMap);
			}
		}
		
		return retVal;
	}
	
	
	
	public ASTDiff map(FunctionDef originalF, FunctionDef newF){
		ITree itree1 = getITreeFromASTNode(originalF);
		ITree itree2 = getITreeFromASTNode(newF);

		Matcher m = Matchers.getInstance().getMatcher(itree1, itree2); // retrieve the default matcher
		m.match();
		MappingStore mappings = m.getMappings(); // return the mapping store
		/*Set<Mapping> currMappings = mappings.asSet();
		for(Mapping currM:currMappings) {
			ITree currA = currM.first;
			ITree currB = currM.second;
			ASTNode aASTN = this.getMappedASTNode(currA);
			ASTNode bASTN = this.getMappedASTNode(currB);
			System.out.println("Mapped:" + aASTN.getEscapedCodeStr() + ", Second:" + bASTN.getEscapedCodeStr());
		}*/
		ActionGenerator g = new ActionGenerator(itree1, itree2, mappings);
		g.generate();
		return new ASTDiff(mappings, g.getActions(), this);
	}
	
	
	public ITree getITreeFromASTNode(ASTNode currAstNode, TreeContext context, ITree parentTree) {
		// Node Type
		String nodeType = currAstNode.getTypeAsString();
		// Node id
		int nodeId = nodeType.hashCode();
		registerTypeLabel(nodeId, nodeType);
		//context.createTree(nodeId, )
		
		// Get label
		String nodeLabel = currAstNode.getLabel();
		
		// get pos
		int pos = currAstNode.getLocation().startIndex;
		// get length
		int length = currAstNode.getLocation().stopIndex - currAstNode.getLocation().startIndex + 1;
		// Get AST Node is attribute
		String astNodeIDAttr = AST_NODE_ID_KEY;
		String astNodeIDVal = Long.toString(currAstNode.getIDForGumTree());
		
		ITree newTree = context.createTree(nodeId, nodeLabel, nodeType);
		newTree.setPos(pos);
		newTree.setLength(length);
		
		// This should not affect algorithm 
		newTree.setMetadata(astNodeIDAttr, astNodeIDVal);
		
		/*
		 * Updating maps for a faster mapping between Joern and Gumtree trees
		 */
		if(astNodeITreeMap.get(currAstNode) != null ||
				iTreeASTNodeMap.get(newTree) != null)
			throw new RuntimeException("Mapping the same node more than one time. A new instance of GumtreeASTMapper()"
					+ "should be created for every ASTDiff.");
		astNodeITreeMap.put(currAstNode, newTree);
		iTreeASTNodeMap.put(newTree, currAstNode);
		
		// process children.
		for(int i =0 ; i < currAstNode.getChildCount() ; i++) {
			ASTNode currChild = currAstNode.getChild(i);
			getITreeFromASTNode(currChild, context, newTree);
		}
		// update the parent tree
		if(parentTree != null) {
			newTree.setParentAndUpdateChildren(parentTree);
		}
		return newTree;
		
	}
	
	public ITree getITreeFromASTNode(ASTNode currAstNode) {
		TreeContext context = new TreeContext();
		ITree rootNode = getITreeFromASTNode(currAstNode, context, null);
		context.setRoot(rootNode);
		context.validate();
		return rootNode;
	}

	public ITree getMappedITree(ASTNode node){
		return astNodeITreeMap.get(node);
	}
	
	public ASTNode getMappedASTNode(ITree iTree){
		return iTreeASTNodeMap.get(iTree);
	}
	
//	NOT MORE USEFUL
	
	/**
//	 * Given a list  of Gumtree actions and an ASTNode
//	 * returns all only the actions in the list that affect it. 
//	 */
//	public static List<Action> filterActionsByNode(List<Action> actions, ASTNode astNode){
//		List<Action> l = new ArrayList<Action>();
//		if(logger.isDebugEnabled()) logger.debug(astNode + " " + astNode.getIDForGumTree());
//		for(Action a : actions){
//			if(logger.isDebugEnabled()) logger.debug(a.getName());
//			if(logger.isDebugEnabled()) logger.debug(a.getNode().getId() + " " + a.getNode().getMetadata(AST_NODE_ID_KEY));
//			if(Long.parseLong((String) a.getNode().getMetadata(AST_NODE_ID_KEY)) == astNode.getIDForGumTree()){
//				if(logger.isDebugEnabled()) logger.debug("ACTION ADDED TO LIST!");
//				l.add(a);
//			}
//			for(ITree t : a.getNode().getDescendants()){
//				if(logger.isDebugEnabled()) logger.debug(t.getId() + " " + t.getMetadata(AST_NODE_ID_KEY));
//				if(Long.parseLong((String) t.getMetadata(AST_NODE_ID_KEY)) == astNode.getIDForGumTree()){
//					if(logger.isDebugEnabled()) logger.debug("ACTION ADDED TO LIST!");
//					l.add(a);
//				}
//			}
//		}
//		return l;
//	}
	
	/*
	 * TEST MAIN
	 */
	public static void main(String[] args) {
		
		ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
		ModuleParser parser = new ModuleParser(driver);
		SafepatchASTWalker astWalker = new SafepatchASTWalker();
		parser.addObserver(astWalker);
		GumtreeASTMapper mapper = new GumtreeASTMapper();
		
		
		/*
		 * TEST - ASTNode to ITree
		 */
		
		String filename = new String("../test_files/test_old.c");
		File file = new File(filename); 
		String preprocessed = file.toPath().toString().replaceAll(file.toPath().getFileName().toString(), 
				SafepatchMain.PREPROCESSED_FILE_PREFIX + file.toPath().getFileName());
		try {
			SafepatchPreprocessor prep = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			prep.preProcess(file.toPath().toString(), preprocessed);
			parser.parseFile(preprocessed);
			ArrayList<FunctionDef> functionsList = astWalker.getFileFunctionList().get(preprocessed);
			for(FunctionDef f : functionsList){
				ITree tree = mapper.getITreeFromASTNode(f);
				System.out.println("\nJOERN AST:" + f.name.getEscapedCodeStr());
				JoernUtil.printASTNodeAndChildren(f);
				System.out.println("\nGUMTREE ITREE: " + f.name.getEscapedCodeStr());
				GumtreeUtil.printITree(tree);
				
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		/*
		 * TEST - ASTDiff and Actions
		 */
		
//		String oldFileName = "../test_files/json_example_1/old.c";
//		String newFileName = "../test_files/json_example_1/new.c";
//		String[] jsonFiles = { "../test_files/json_example_1/json.json" };
//		
//		
//		File oldFile = new File(oldFileName);
//		File newFile = new File(newFileName);
//		Path oldPath = oldFile.toPath();
//		Path newPath = newFile.toPath();
//
//		String oldPreprocessed = oldPath.toString().replaceAll(oldPath.getFileName().toString(), 
//				SafepatchMain.PREPROCESSED_FILE_PREFIX + oldPath.getFileName());
//		String newPreprocessed = newPath.toString().replaceAll(newPath.getFileName().toString(),
//				SafepatchMain.PREPROCESSED_FILE_PREFIX + newPath.getFileName());
//		try {
//			SafepatchPreprocessor.PreProcess(oldPath.toString(), oldPreprocessed);
//			SafepatchPreprocessor.PreProcess(newPath.toString(), newPreprocessed);
//			parser.parseFile(oldPreprocessed);
//			parser.parseFile(newPreprocessed);
//			ArrayList<FunctionDef> oldFunctionList = astWalker.getFileFunctionList().get(oldPreprocessed);
//			ArrayList<FunctionDef> newFunctionList = astWalker.getFileFunctionList().get(newPreprocessed);
//			FileDiff diff = new FileDiff(oldFile, newFile);
//			diff.diff();
//			
//			Files.delete(Paths.get(oldPreprocessed));
//			Files.delete(Paths.get(newPreprocessed));
//			
//			ArrayList<FunctionASTDiff> commonFunctionsDiffs = new ArrayList<>();
//			for(FunctionDef oldFunc: oldFunctionList){
//				for(FunctionDef newFunc : newFunctionList){
//					if (SafepatchMain.isSameFunction(oldFunc, newFunc)){ 
//						
//						FunctionASTDiff f = new FunctionASTDiff(oldFile, oldFunc, newFile, newFunc);
//						f.diff(diff);
//						commonFunctionsDiffs.add(f);
//					}
//				}
//			}
//			
//			GumtreeASTMapper mapper = new GumtreeASTMapper();
//			for (FunctionASTDiff f: commonFunctionsDiffs){
//				List<ASTDelta> deltas = f.getAstDeltas();
//				for (int i = 0 ; i < deltas.size() ; i++){
//					ASTDelta d = deltas.get(i);
//					//GumtreeDiffReader reader = new GumtreeDiffReader();
//					//GumtreeDiff g = reader.readFromJson(jsonFiles[i]);
//					
//					mapper.map(d);
//				}
//			}
//			
//		} catch (IOException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}
		 
	}
	

}
