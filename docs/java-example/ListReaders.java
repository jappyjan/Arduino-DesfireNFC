import javax.smartcardio.*;
import java.util.List;

public class ListReaders {
    public static void main(String[] args) {
        try {
            TerminalFactory factory = TerminalFactory.getDefault();
            List<CardTerminal> terminals = factory.terminals().list();
            if (terminals.isEmpty()) {
                System.out.println("No PC/SC terminals found.");
            } else {
                System.out.println("Available PC/SC terminals:");
                for (CardTerminal terminal : terminals) {
                    System.out.println("- " + terminal.getName());
                }
            }
        } catch (Exception e) {
            System.err.println("Error listing terminals: " + e.getMessage());
            e.printStackTrace();
        }
    }
} 