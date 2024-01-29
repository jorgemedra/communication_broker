

class MyClass{

  constructor() {
    console.debug('MyClass::constructor');
    this.callbavk_fn = null;
    this.intv = null;
  }

  setCBF(cbf) {
    console.debug('MyClass::setCBF');
    this.callbavk_fn = cbf;
  }

  start()
  {
    console.debug('start');
    this.intv = setInterval(()=>{
      console.debug('MyClass::start::interval');
      this.callbavk_fn('This is my message from \'mylib.js\'.');
      clearInterval(this.intv);
    }, 3000);
  }

};
