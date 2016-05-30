/*
*
* Name: keypad.cpp
*
*/

#include "keypad.h"

// State:
char KeyState;
// Bit pattern after each scan:
char KeyCode;
// Output value from the virtual 74HC922:
KeyPressed KeyValue;
// KeyDown is set if key is down:
char KeyDown;
// KeyNew is set every time a new key is down:
char KeyNew;
 
// Pin of Row
uint16_t _rows[4] = {KEYx0, KEYx1, KEYx2, KEYx3};
uint16_t _cols[3] = {KEYy0, KEYy1, KEYy2};

//���캯��
void Keypad(void)
{
  
  Stop();   
  KeyState = 0; // ����״̬��ʼ 0
}


//ɨ�����
void Scan_Keyboard(void){
  switch (KeyState) {
  case 0: {
    if (AnyKey()) {    
      char scankey = ScanKey();
      if (scankey != 0xff)
        KeyCode = scankey;
      
      KeyState = 1;
    }
    
    break;
  }
  case 1: {
    if (SameKey()) {
      FindKey();
      KeyState = 2;  
    }
    else 
      KeyState = 0;
    
    break;
  }
  case 2: {
    if (SameKey()) {
    }
    else 
      KeyState = 3;
    
    break;
  }
  case 3: {
    if (SameKey()) {
      KeyState = 2;
    }
    else {
      ClearKey();
      KeyState = 0;
    }
    
    break;
  }
  }
  // func end
}

// �м�����
char AnyKey(void) {
  //Start();  //����  
  int r = -1;
  for (r = 0; r < row_count; r++) {
    if (In(_rows[r]) == 0)  // In macro
      break;	 
  }
  
  //Stop();  //�ָ�
  
  if (!(0 <= r && r<row_count))
    return 0;
  else
    return 1;
}

// �����£� ��ֵ��ͬ
char SameKey(void) {
  
  // char KeyCode_new = KeyCode;
  char KeyCode_new = ScanKey();
  if (KeyCode == KeyCode_new)  
    return 1;
  else
    return 0;
}

// ɨ���
char ScanKey(void) {
  
  /* ��ɨ�� */
  int r = -1;
  for (r=0; r<row_count; r++) {
    if (In(_rows[r]) == 0)  // In macro
      break;  
  }
  
  /* ��û���ҵ���Ч�У����� */
  if (!(0 <= r && r < row_count)) {
    return 0xff;
    
  }
  
  /* ��ɨ�裬�ҳ������ĸ������� */
  int c = -1;
  for (c=0; c<col_count; c++) {
    // �����������
    Cols_out(~(1<<c));    
    if (In(_rows[r]) == 0)  // In macro
      break;
  }
  
  /* �����е������³�� */
  Start();
  
  /* ��û���ҵ���Ч�У����� */
  if (!(0 <= c && c<col_count))
    return 0xff;

  return r * col_count + c;
  
}

// FindKey compares KeyCode to values in KeyTable.
// If match, KeyValue, KeyDown and KeyNew are updated.
void FindKey(void) {
  KeyValue = (KeyPressed)KeyCode;
  KeyDown = 1;
  KeyNew = 1;
}


void ClearKey(void) {
  KeyDown = 0;
}

KeyPressed getKey(void) {
  if(KeyNew)
  {
    KeyNew = 0;
    return KeyValue;
  }
  else
    return Key_None;
}

void Start(void)
{
  /* �����0�� ������ */
  PORT_KEY->BRR = COLS;
}

void Stop(void)
{
  /* �����1��ʹ�в������� */
  PORT_KEY->BSRR = COLS;
}

// cols bus output
void Cols_out(uint16_t v)
{
  v &= 0x07;
  
  if (v == 0x6) //0b001
    //High(_cols[0]);
    PORT_KEY->BSRR = (uint32_t)_cols[0] << 16;
  else
    //Low(_cols[0]);
    PORT_KEY->BSRR = _cols[0];
  
  if (v == 0x5) //0b010
    //High(_cols[1]);
    PORT_KEY->BSRR = (uint32_t)_cols[1] << 16;
  else
    //Low(_cols[1]);
    PORT_KEY->BSRR = _cols[1];
  
  if (v == 0x3) //0b100
    //High(_cols[2]);
    PORT_KEY->BSRR = (uint32_t)_cols[2] << 16;
  else
    //Low(_cols[2]);
    PORT_KEY->BSRR = _cols[2];
  
}

/* ����Ir����ӳ�� */
extern osMessageQId Q_IrHandle;

KeyPressed getIrKey(void)
{
  osEvent evt = osMessageGet(Q_IrHandle, 0);
  KeyPressed key_pressed = Key_None;
  
  if (evt.status == osEventMessage) {
    // ��Q��ȡֵ
    uint8_t v = (uint8_t)evt.value.v;
    // ����ӳ��
    switch(v) {
    case 0x5:
      key_pressed = Key_Up;
      break;
    case 0xd:
      key_pressed = Key_Down;
      break;
    case 0x8:
      key_pressed = Key_Left;
      break;
    case 0xa:
      key_pressed = Key_Right;
      break;
    case 0x10:
      key_pressed = Key_Power;
      break;
    case 0x11:
      key_pressed = Key_Menu;
      break;
    case 0x12:
      key_pressed = Key_Mode;
      break;
    }    
  }
  return key_pressed;
}
