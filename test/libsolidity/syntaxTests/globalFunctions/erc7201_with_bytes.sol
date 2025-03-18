bytes constant b = "abcdef";
contract C {
    function f() public pure returns (uint256) {
        return erc7201(b);
    }
}
// ----
// TypeError 6896: (114-115): The argument of builtin erc7201 must be either a constant string variable or a string literal.
