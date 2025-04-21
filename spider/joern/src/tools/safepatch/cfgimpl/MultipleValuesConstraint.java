package tools.safepatch.cfgimpl;

import java.util.HashMap;
import java.util.Map;

import ast.ASTNode;
import ast.statements.Condition;
import cfg.nodes.ASTNodeContainer;

public class MultipleValuesConstraint {

	private ASTNodeContainer condition;
	private Map<String, ASTNode> alternatives = new HashMap<String, ASTNode>();
	
	public MultipleValuesConstraint(ASTNodeContainer astNodeContainer) {
		super();
		this.condition = astNodeContainer;
	}
	
	public void addAlternative(String conditionEquals, ASTNode value){
		this.alternatives.put(conditionEquals, value);
	}

	public ASTNodeContainer getCondition() {
		return condition;
	}

	public Map<String, ASTNode> getAlternatives() {
		return alternatives;
	}
	
}
