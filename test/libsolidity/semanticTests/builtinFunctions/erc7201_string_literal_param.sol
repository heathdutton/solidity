contract C {
    function stringLiteral() public pure returns (uint) {
        return erc7201("erc7201:example.main");
    }
    function emptyString() public pure returns (uint) {
        return erc7201("");
    }
}
// ----
// stringLiteral() -> 42181338000572692212425743771350556927907101704784250875717988292114786466048
// emptyString() -> 30348469548119976384149824193117947667795829812057172845188107037932402691072
