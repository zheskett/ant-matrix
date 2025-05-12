import { MainScene } from "./scenes/main";
import { Manager } from "./util/manager";

(async () => {
  await Manager.initialize();

  Manager.setScene(new MainScene());
})();
