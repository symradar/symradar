package tools.safepatch.util;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.github.gumtreediff.actions.model.Action;
import com.github.gumtreediff.actions.model.Move;
import com.github.gumtreediff.actions.model.Update;
import com.github.gumtreediff.matchers.Mapping;
import com.github.gumtreediff.tree.ITree;

import ast.ASTNode;
import tools.safepatch.fgdiff.GumtreeASTMapper;

/**
 * @author Eric Camellini
 *
 */
public class GumtreeUtil {
	
	
	
	public static void printActions(List<Action> actions){
		for(Action a : actions){			
			System.out.println(a.getName());
			printITree(a.getNode());
			System.out.println();
		}
	}
	
	public static void printITree(ITree tree){
		printITree(tree, 0);
	}
	
	public static void printITree(ITree tree, int indent){
		printITreeNode(tree, indent);
		indent++;
		for(ITree child : tree.getChildren())
			printITree(child, indent);
	}
	
	public static void printITreeNode(ITree tree, int indent){
		System.out.println(Util.repeatString(" ", indent*2)
				+ "Id: " + tree.getId()
				+ " - Label: " + tree.getLabel()
				+ " - Type: " + tree.getType()
				+ " - TypeLabel: " + GumtreeASTMapper.getTypeLabel(tree.getType())
				+ " - Pos: " + tree.getPos()
				+ " - Length:" + tree.getLength()
				+ " - EndPos: " + tree.getEndPos()
				+ " - ASTNode id: " + tree.getMetadata(GumtreeASTMapper.AST_NODE_ID_KEY));
	}
	
	/*
	 * OLD STUFF:
	 */
	
//	public static void printGumtreeDiff(GumtreeDiff d){
//		System.out.println("OLD:");
//		printGumtreeActions(d.getOldActions());
//		System.out.println("NEW:");
//		printGumtreeActions(d.getNewActions());
//	}
//	
//	public static void printGumtreeActions(List<GumtreeAction> l){
//		for(GumtreeAction a : l)
//			printGumtreeAction(a);
//	}
//	 
//	public static void printGumtreeAction(GumtreeAction a){
//		System.out.println("action: " + a.getType());
//		System.out.println("startLine: " + a.getStartLine());
//		System.out.println("startColumn: " + a.getStartColumn());
//		System.out.println("endLine: " + a.getEndLine());
//		System.out.println("endColumn: " + a.getEndColumn());
//		System.out.println("text: " + a.getText());
//		System.out.println("dataTitle: " + a.getDataTitle());
//		System.out.println("children: ["); 
//		printGumtreeActions(a.getChildren());
//		System.out.println("]");
//	}	
//	
//	public static void printGumtreeDiff(wrongFormat d){
//		System.out.println("action: " + d.getAction());
//		System.out.println("oldStartLine: " + d.getPosition().getOldStartLine());
//		System.out.println("oldStartColumn: " + d.getPosition().getOldStartColumn());
//		System.out.println("oldEndLine: " + d.getPosition().getOldEndLine());
//		System.out.println("oldEndColumn: " + d.getPosition().getOldEndColumn());
//		System.out.println("newStartLine: " + d.getPosition().getNewStartLine());
//		System.out.println("newStartColumn: " + d.getPosition().getNewStartColumn());
//		System.out.println("newEndLine: " + d.getPosition().getNewEndLine());
//		System.out.println("newEndColumn: " + d.getPosition().getNewEndColumn());
//		System.out.println("oldText: " + d.getText().getOldVersion());
//		System.out.println("newText: " + d.getText().getNewVersion());
//		System.out.println("type: " + d.getType());
//		System.out.println("children: ["); 
//		for (wrongFormat c : d.getChildren())
//			printGumtreeDiff(c);
//		System.out.println("]");
//	}
//	
//	public static List<wrongFormat> readGumtreeDiffFromJson(String file) throws JsonParseException, JsonMappingException, IOException{
//		ObjectMapper mapper = new ObjectMapper(); //Reusable
//		List<wrongFormat> gumtreeDiffs= Arrays.asList(mapper.readValue(new File(file), wrongFormat[].class));
//		return gumtreeDiffs;
//	}
//	
//	public static List<wrongFormat> unNest(wrongFormat g, Boolean cloneObjects) throws CloneNotSupportedException{
//		/**
//		 * Method to unNest the GumtreeDiff extracted from the JSON so that all the 
//		 * objects can be found in the resulting list without having to access the children
//		 */
//		List<wrongFormat> l = new ArrayList<wrongFormat>();
//		if(cloneObjects){
//			/*
//			 * In this case objects are cloned and their child are removed
//			 */
//			unNestCloneHelper(g, l);
//		} else {
//			/*
//			 * In this case the List references the original objects so the children are
//			 * still accessible, even if they will be already present within the unnested list
//			 */
//			unNestHelper(g, l);
//		}
//		return l;
//	}
//	
//	private static void unNestCloneHelper(wrongFormat g, List<wrongFormat> l) throws CloneNotSupportedException{
//		wrongFormat clone = (wrongFormat) g.clone();
//		l.add(clone);
//		for (wrongFormat child : clone.getChildren())
//			unNestCloneHelper(child, l);
//		clone.setChildren(new ArrayList<>());
//		
//	}
//	
//	private static void unNestHelper(wrongFormat g, List<wrongFormat> l){
//		l.add(g);
//		for (wrongFormat child : g.getChildren())
//			unNestHelper(child, l);
//	}
//	
//	/*
//	 * TEST MAIN:
//	 */
//	public static void main(String[] args) {
//		try {
//			List<wrongFormat> l = readGumtreeDiffFromJson("../test_files/test1_gumtree_json_diff.json");
//			System.out.println("ORIGINAL GUMTREEDIFF:");
//			for (wrongFormat d : l)
//				printGumtreeDiff(d);
//			
//			List<wrongFormat> unNested = new ArrayList<wrongFormat>();
//			for (wrongFormat d : l)
//				unNested.addAll(unNest(d, false));
//			
//			System.out.println("UN-NESTED GUMTREEDIFF:");
//			for (wrongFormat d : unNested)
//				printGumtreeDiff(d);
//			
//		} catch (IOException | CloneNotSupportedException e) {
//			e.printStackTrace();
//		}
//	}
	
}
