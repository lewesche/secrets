import "./style.css"
import "./secrets.css"
import { init } from "./app/app"
import { goButton } from "./app/go";
import { newUserButton } from "./app/newUser";

const go = new goButton();
const newUser = new newUserButton();

init()

