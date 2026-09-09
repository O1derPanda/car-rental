modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionInterrogateBot);
        actions.Insert(ActionRestrainMoraleBot);
    }
}

modded class PlayerBase
{
    override void SetActions(out TInputActionMap InputActionMap)
    {
        super.SetActions(InputActionMap);

        AddAction(ActionInterrogateBot, InputActionMap);
        AddAction(ActionRestrainMoraleBot, InputActionMap);
    }
}
