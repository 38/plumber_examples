# Typed Guest Code Example

This is the example for the typed guest code. The component `parse-input` parses the line-based text to structured data.
And the structured data is added up with the `adder` component.

The communication protocol between those components are defined with the protocol description DSL in `adder.ptype`.

To run the example, just simply run

```bash
pscript adder.pss
```

This example also demostrate the generic typing with Plumber. 
The JSON component in Plumber is a good example for generic, which doesn't assume any protocol in the code.
The protocol is loaded when the component is initialized.

To try the example, you can run

```
bash dump-json.pss
```
