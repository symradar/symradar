package ast.expressions;

public class CastTarget extends Expression
{
	@Override
	public String getLabel() {
		return this.getEscapedCodeStr();
	}
}
