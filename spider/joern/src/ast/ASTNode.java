package ast;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

import org.antlr.v4.runtime.ParserRuleContext;

import com.google.common.collect.Lists;

import parsing.ParseTreeUtils;
import ast.expressions.BinaryExpression;
import ast.expressions.Expression;
import ast.walking.ASTNodeVisitor;

public class ASTNode implements Cloneable
{

	protected String codeStr = null;
	protected ParserRuleContext parseTreeNodeContext;
	private CodeLocation location = new CodeLocation();

	private boolean isInCFG = false;

	protected LinkedList<ASTNode> children;
	protected LinkedList<ASTNode> descendants;
	protected LinkedList<ASTNode> nodes;
	protected int childNumber;
	
	public static long UNINIT_UNIQUE_ID = 0xdeadbeef;
	public static boolean enableHashCode = false;
	
	public long uniqueid = UNINIT_UNIQUE_ID;
	private ASTNode parent = null;
	
	public void addChild(ASTNode node)
	{
		if (children == null)
			children = new LinkedList<ASTNode>();
		node.setChildNumber(children.size());
		children.add(node);
		node.setParent(this);
	}

	public int getChildCount()
	{
		if (children == null)
			return 0;
		return children.size();
	}

	public ASTNode getChild(int i)
	{
		if (children == null)
			return null;

		ASTNode retval;
		try
		{
			retval = children.get(i);
		}
		catch (IndexOutOfBoundsException ex)
		{
			return null;
		}
		return retval;
	}

	public ASTNode popLastChild()
	{
		return children.removeLast();
	}

	public void setChildNumber(int num)
	{
		childNumber = num;
	}

	public int getChildNumber()
	{
		return childNumber;
	}

	public void initializeFromContext(ParserRuleContext ctx)
	{
		parseTreeNodeContext = ctx;
	}

	public void setLocation(ParserRuleContext ctx)
	{
		if (ctx == null)
			return;
		location = new CodeLocation(ctx);
	}

	public void setCodeStr(String aCodeStr)
	{
		codeStr = aCodeStr;
	}

	public String getEscapedCodeStr()
	{
		if (codeStr != null)
			return codeStr;

		codeStr = escapeCodeStr(ParseTreeUtils
				.childTokenString(parseTreeNodeContext));
		return codeStr;
	}

	private String escapeCodeStr(String codeStr)
	{
		String retval = codeStr;
		retval = retval.replace("\n", "\\n");
		retval = retval.replace("\t", "\\t");
		return retval;
	}

	public String getLocationString()
	{
		setLocation(parseTreeNodeContext);
		return location.toString();
	}
	
	public String getCompleteCodeContent() {
		String toRet = this.getEscapedCodeStr();
		if(this.children != null) {
			for(ASTNode currChild:this.children) {
				if(currChild != null) {
					toRet += currChild.getCompleteCodeContent();
				}
			}
		}
		return toRet;
	}

	public CodeLocation getLocation()
	{
		setLocation(parseTreeNodeContext);
		return location;
	}

	
	public void accept(ASTNodeVisitor visitor)
	{
		visitor.visit(this);
	}

	public boolean isLeaf()
	{
		return (getChildCount() == 0);
	}

	public boolean isRoot() {
        return getParent() == null;
    }
	
	public ASTNode getRoot(){
		if(isRoot())
			return this;
		else
			return (getParent().getRoot());
	}
	
	public String getTypeAsString()
	{
		return this.getClass().getSimpleName();
	}
	
	// This is required for gumtree diff and for code generation
	public String getLabel() 
	{
		return "";
	}
	
	public long getIDForGumTree() 
	{
		if(uniqueid == UNINIT_UNIQUE_ID) 
		{
			// randomly generate a unique id
			Random r = new Random();
			uniqueid = r.nextLong();
					
		}
		return uniqueid;
		
	}

	public void markAsCFGNode()
	{
		isInCFG = true;
	}

	public boolean isInCFG()
	{
		return isInCFG;
	}

	public String getOperatorCode()
	{
		if (Expression.class.isAssignableFrom(this.getClass()))
		{
			return ((Expression) this).getOperator();
		}
		return null;
	}

	public LinkedList<ASTNode> getChildren(){
		return this.children;
	}
	
	
	public ASTNode getParent() {
		return parent;
	}

	public void setParent(ASTNode parent) {
		this.parent = parent;
	}

	public LinkedList<ASTNode> getParents(){
		LinkedList<ASTNode> parents = new LinkedList<ASTNode>();
		if(getParent() == null) return parents;
		else{
			parents.add(getParent());
			parents.addAll(getParent().getParents());
		}
		return parents;
	}

	public List<ASTNode> getParentsReversed(){
		return (List<ASTNode>) Lists.reverse(getParents());
	}
	
	public LinkedList<ASTNode> getDescendants(){
		if(descendants == null){
			descendants = new LinkedList<ASTNode>();
			addDescendants(this, descendants);
		}
		return descendants;
	}
	
	private void addDescendants(ASTNode n, LinkedList<ASTNode> descendants){
		if (n.getChildCount() != 0){
			for(int i = 0 ; i < n.getChildCount() ; i++){
				descendants.add(n.getChild(i));
				addDescendants(n.getChild(i), descendants);
			}
		}
	}
	
	/**
	 * Returns a list containing the node itself plus all the descendants
	 */
	public LinkedList<ASTNode> getNodes(){
		if(nodes == null){
			nodes = new LinkedList<ASTNode>();
			nodes.addAll(getDescendants());
			nodes.add(0, this);
		}
		return nodes;
	}
	
	public ArrayList<ASTNode> getLeaves(){
		ArrayList<ASTNode> leaves = new ArrayList<ASTNode>();
		for(ASTNode n : getNodes())
			if(n.isLeaf()) leaves.add(n);
		return leaves;
	}
	
	public ASTNode clone() throws CloneNotSupportedException
	{
		ASTNode node = (ASTNode)super.clone();
		node.setCodeStr(codeStr);
		node.initializeFromContext(parseTreeNodeContext);
		if(isInCFG())
			node.markAsCFGNode();
		if(children != null)
		{
			for(ASTNode n:children)
			{
				node.addChild(n.clone());
			}
		}
		node.setChildNumber(childNumber);
		return node;
	}

	@Override
	public String toString() {
		return super.toString() + "_code[" + this.getEscapedCodeStr() + "]";
		//return this.getClass().getSimpleName() + "@" + this.hashCode() + "_code[" + this.getEscapedCodeStr() + "]";
	}
	
	@Override
	public int hashCode() {
		if(this.enableHashCode) {
			return this.parseTreeNodeContext.hashCode() ^ this.location.hashCode();
		}
		return super.hashCode();
	}
	
	@Override
	public boolean equals(Object obj1) {
		if(this.enableHashCode) {
			if(obj1 instanceof ASTNode) {
				ASTNode other = (ASTNode)obj1;
				if(this.getIDForGumTree() == other.getIDForGumTree()) {
					if(this.getParent() == null || other.getParent() == null) {
						return this.getParent() == null && other.getParent() == null;
					}
					return this.getParent().getIDForGumTree() == other.getParent().getIDForGumTree();
				}
			}
			return false;
		}
		return super.equals(obj1);
	}
	
}
